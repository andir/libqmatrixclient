/******************************************************************************
 * Copyright (C) 2016 Felix Rohrbach <kde@fxrh.de>
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

#include "roommessagesjob.h"
#include "../room.h"
#include "../events/event.h"
#include "../json.h"

using namespace QMatrixClient;

GetRoomMessages::GetRoomMessages(Room* room, QString from, FetchDirection dir, int limit)
    : APIParams("GetRoomMessages", JobHttpType::GetJob,
        Endpoint(QString("/_matrix/client/r0/rooms/%1/messages").arg(room->id())),
        Query(
            { { "from", from }
            , { "limit", QString::number(limit) }
            , { "dir", dir == FetchDirection::Backwards ? "b" : "f" }
            }
        ))
{ }

GetRoomMessages::Result::Result(const QJsonDocument& data)
{
    JsonObject obj { data };
    events = eventListFromJson(obj["chunk"].toArray());
    obj.assign("end", &end);
}
