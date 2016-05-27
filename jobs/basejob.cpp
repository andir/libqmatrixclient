/******************************************************************************
 * Copyright (C) 2015 Felix Rohrbach <kde@fxrh.de>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "basejob.h"

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtCore/QTimer>

#include "../connectiondata.h"

using namespace QMatrixClient;

class BaseJob::Private
{
    public:
        Private(ConnectionData* c, JobHttpType t, bool nt)
            : connection(c), reply(nullptr), type(t), needsToken(nt), errorCode(NoError)
        {}
        
        ConnectionData* connection;
        QNetworkReply* reply;
        JobHttpType type;
        bool needsToken;

        int errorCode;
        QString errorText;
};

BaseJob::BaseJob(ConnectionData* connection, JobHttpType type, QString name, bool needsToken)
    : d(new Private(connection, type, needsToken))
{
    setObjectName(name);
}

BaseJob::~BaseJob()
{
    if( d->reply )
    {
        if( d->reply->isRunning() )
            d->reply->abort();
        d->reply->deleteLater();
    }
    delete d;
}

ConnectionData* BaseJob::connection() const
{
    return d->connection;
}

QJsonObject BaseJob::data() const
{
    return QJsonObject();
}

QUrlQuery BaseJob::query() const
{
    return QUrlQuery();
}

void BaseJob::parseJson(const QJsonDocument& data)
{
}

void BaseJob::start()
{
    QUrl url = d->connection->baseUrl();
    url.setPath( url.path() + "/" + apiPath() );
    QUrlQuery query = this->query();
    if( d->needsToken )
        query.addQueryItem("access_token", connection()->token());
    url.setQuery(query);
    QNetworkRequest req = QNetworkRequest(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
#if (QT_VERSION >= QT_VERSION_CHECK(5, 6, 0))
    req.setAttribute(QNetworkRequest::FollowRedirectsAttribute, true);
    req.setMaximumRedirectsAllowed(10);
#endif
    QJsonDocument data = QJsonDocument(this->data());
    switch( d->type )
    {
        case JobHttpType::GetJob:
            d->reply = d->connection->nam()->get(req);
            break;
        case JobHttpType::PostJob:
            d->reply = d->connection->nam()->post(req, data.toJson());
            break;
        case JobHttpType::PutJob:
            d->reply = d->connection->nam()->put(req, data.toJson());
            break;
    }
    connect( d->reply, &QNetworkReply::sslErrors, this, &BaseJob::sslErrors );
    connect( d->reply, &QNetworkReply::finished, this, &BaseJob::gotReply );
    QTimer::singleShot( 120*1000, this, SLOT(timeout()) );
//     connect( d->reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
//              this, &BaseJob::networkError ); // http://doc.qt.io/qt-5/qnetworkreply.html#error-1
}

void BaseJob::finishJob(bool emitResult)
{
    if( d->reply->isRunning() )
        d->reply->abort();

    // Notify those that are interested in any completion of the job (including killing)
    emit finished(this);

    if (emitResult) {
        emit result(this);
        if (error())
            emit failure(this);
        else
            emit success(this);
    }

    deleteLater();
}

bool BaseJob::kill()
{
    finishJob(false);
    return true;
}

void BaseJob::setError(int errorCode)
{
    d->errorCode = errorCode;
}

void BaseJob::setErrorText(QString errorText)
{
    d->errorText = errorText;
}

void BaseJob::emitResult()
{
    finishJob(true);
}

int BaseJob::error() const
{
    return d->errorCode;
}

QString BaseJob::errorString() const
{
    return d->errorText;
}

void BaseJob::fail(int errorCode, QString errorString)
{
    setError( errorCode );
    setErrorText( errorString );
    if( d->reply->isRunning() )
        d->reply->abort();
    qWarning() << "Job" << objectName() << "failed:" << errorString;
    emitResult();
}

QNetworkReply* BaseJob::networkReply() const
{
    return d->reply;
}

// void BaseJob::networkError(QNetworkReply::NetworkError code)
// {
//     fail( KJob::UserDefinedError+1, d->reply->errorString() );
// }

void BaseJob::gotReply()
{
    if( d->reply->error() != QNetworkReply::NoError )
    {
        qDebug() << "NetworkError:" << d->reply->error();
        fail( NetworkError, d->reply->errorString() );
        return;
    }
    QJsonParseError error;
    QJsonDocument data = QJsonDocument::fromJson(d->reply->readAll(), &error);
    if( error.error != QJsonParseError::NoError )
    {
        fail( JsonParseError, error.errorString() );
        return;
    }
    parseJson(data);
}

void BaseJob::timeout()
{
    fail( TimeoutError, "The job has timed out" );
}

void BaseJob::sslErrors(const QList<QSslError>& errors)
{
    foreach (const QSslError &error, errors) {
        qWarning() << "SSL ERROR" << error.errorString();
    }
    d->reply->ignoreSslErrors(); // TODO: insecure! should prompt user first
}
