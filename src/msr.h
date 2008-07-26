/*
 * libdlna: reference DLNA standards implementation.
 * Copyright (C) 2007-2008 Benjamin Zores <ben@geexbox.org>
 *
 * This file is part of libdlna.
 *
 * libdlna is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * libdlna is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with libdlna; if not, write to the Free Software
 * Foundation, Inc, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef MSR_H
#define MSR_H

#define MSR_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">" \
"<specVersion>" \
"  <major>1</major>" \
"  <minor>0</minor>" \
"</specVersion>" \
"<actionList>" \
"  <action>" \
"    <name>IsAuthorized</name>" \
"    <argumentList>" \
"      <argument>" \
"        <name>DeviceID</name>" \
"        <direction>in</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_DeviceID</relatedStateVariable>" \
"      </argument>" \
"      <argument>" \
"        <name>Result</name>" \
"        <direction>out</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>" \
"      </argument>" \
"    </argumentList>" \
"  </action>" \
"  <action>" \
"    <name>RegisterDevice</name>" \
"    <argumentList>" \
"      <argument>" \
"        <name>RegistrationReqMsg</name>" \
"        <direction>in</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_RegistrationReqMsg</relatedStateVariable>" \
"      </argument>" \
"      <argument>" \
"        <name>RegistrationRespMsg</name>" \
"        <direction>out</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_RegistrationRespMsg</relatedStateVariable>" \
"      </argument>" \
"    </argumentList>" \
"  </action>" \
"  <action>" \
"    <name>IsValidated</name>" \
"    <argumentList>" \
"      <argument>" \
"        <name>DeviceID</name>" \
"        <direction>in</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_DeviceID</relatedStateVariable>" \
"      </argument>" \
"      <argument>" \
"        <name>Result</name>" \
"        <direction>out</direction>" \
"        <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>" \
"      </argument>" \
"    </argumentList>" \
"  </action>" \
"</actionList>" \
"<serviceStateTable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>A_ARG_TYPE_DeviceID</name>" \
"    <dataType>string</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>A_ARG_TYPE_Result</name>" \
"    <dataType>int</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>A_ARG_TYPE_RegistrationReqMsg</name>" \
"    <dataType>bin.base64</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>A_ARG_TYPE_RegistrationRespMsg</name>" \
"    <dataType>bin.base64</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>AuthorizationGrantedUpdateID</name>" \
"    <dataType>ui4</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>AuthorizationDeniedUpdateID</name>" \
"    <dataType>ui4</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>ValidationSucceededUpdateID</name>" \
"    <dataType>ui4</dataType>" \
"  </stateVariable>" \
"  <stateVariable sendEvents=\"no\">" \
"    <name>ValidationRevokedUpdateID</name>" \
"    <dataType>ui4</dataType>" \
"  </stateVariable>" \
"</serviceStateTable>" \
"</scpd>"

#define MSR_DESCRIPTION_LEN strlen (MSR_DESCRIPTION)

#define MSR_LOCATION "/services/msr.xml"

#define MSR_SERVICE_ID "urn:microsoft.com:serviceId:X_MS_MediaReceiverRegistrar"
#define MSR_SERVICE_TYPE "urn:microsoft.com:service:X_MS_MediaReceiverRegistrar:1"

#define MSR_URL              "msr.xml"
#define MSR_CONTROL_URL      "msr_control"
#define MSR_EVENT_URL        "msr_event"

#endif /* MSR_H */
