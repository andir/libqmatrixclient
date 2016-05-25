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

#include "passwordlogin.h"

#include "../json.h"

using namespace QMatrixClient;

PasswordLogin::PasswordLogin(QString user, QString password)
    : APIParams("PasswordLogin", JobHttpType::PostJob
        , "_matrix/client/r0/login"
        , Query()
        , Data({
            { "type", "m.login.password" },
            { "user", user },
            { "password", password }
        }))
{ }

PasswordLogin::Result::Result(const QJsonDocument& data)
{
    JsonObject o { data };
    setValid(o.contains({"access_token", "user_id", "home_server"}));
    fill(o, "access_token", &token);
    fill(o, "user_id", &id);
    fill(o, "home_server", &server);
}
