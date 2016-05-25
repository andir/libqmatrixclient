#include "simplejob.h"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonValue>

using namespace QMatrixClient;

QVariant APIResult::operator[](QString jsonKey) const
{
    return results[jsonKey];
}

void APIResult::setType(QString jsonKey, QVariant::Type type)
{
    results[jsonKey] = QVariant(type);
}

QString QMatrixClient::APIResult::parse(const QJsonDocument& data)
{
    QStringList failedKeys = fillResult(data);
    if (!failedKeys.isEmpty())
        return "Failed to load keys: "+ failedKeys.join(", ");
    
    return QString();
}

QStringList APIResult::fillResult(const QJsonDocument& data)
{
    QStringList failed;
    QJsonObject json = data.object();
    for (auto pItem = results.begin(); pItem != results.end(); ++pItem)
    {
        QJsonValue jsonVal = json[pItem.key()];
        if (jsonVal.type() == QJsonValue::Undefined)
        {
            qDebug() << "Couldn't find JSON value for key" << pItem.key();
            failed.push_back(pItem.key());
            continue;
        }
        // We need a temporary here because QVariant::convert()
        // is very intrusive: if it fails, it resets the QVariant
        // type - and we need to preserve the QVariant type inside
        // the result hashmap.
        QVariant v = jsonVal.toVariant();
        if (v.convert(pItem->userType()))
            *pItem = v;
        else
        {
            qDebug() << "Couldn't convert" << jsonVal << "to" << pItem->type();
            failed.push_back(pItem.key());
        }
    }

    return failed;
}
