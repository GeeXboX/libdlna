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

#ifndef CDS_H
#define CDS_H

#define CDS_DESCRIPTION \
"<?xml version=\"1.0\" encoding=\"utf-8\"?>" \
"<scpd xmlns=\"urn:schemas-upnp-org:service-1-0\">" \
"  <specVersion>" \
"    <major>1</major>" \
"    <minor>0</minor>" \
"  </specVersion>" \
"  <actionList>" \
"    <action>" \
"      <name>GetSearchCapabilities</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>SearchCaps</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SearchCapabilities</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>GetSortCapabilities</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>SortCaps</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SortCapabilities</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>GetSystemUpdateID</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>Id</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>SystemUpdateID</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>Browse</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>ObjectID</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>BrowseFlag</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_BrowseFlag</relatedStateVariable>" \
"       </argument>" \
"        <argument>" \
"          <name>Filter</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>StartingIndex</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>RequestedCount</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>SortCriteria</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>" \
"       </argument>" \
"        <argument>" \
"          <name>Result</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>NumberReturned</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>TotalMatches</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>UpdateID</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"    <action>" \
"      <name>Search</name>" \
"      <argumentList>" \
"        <argument>" \
"          <name>ContainerID</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_ObjectID</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>SearchCriteria</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_SearchCriteria</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>Filter</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Filter</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>StartingIndex</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Index</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>RequestedCount</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>SortCriteria</name>" \
"          <direction>in</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_SortCriteria</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>Result</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Result</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>NumberReturned</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>TotalMatches</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_Count</relatedStateVariable>" \
"        </argument>" \
"        <argument>" \
"          <name>UpdateID</name>" \
"          <direction>out</direction>" \
"          <relatedStateVariable>A_ARG_TYPE_UpdateID</relatedStateVariable>" \
"        </argument>" \
"      </argumentList>" \
"    </action>" \
"  </actionList>" \
"  <serviceStateTable>" \
"    <stateVariable sendEvents=\"yes\">" \
"      <name>TransferIDs</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_ObjectID</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Result</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_SearchCriteria</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_BrowseFlag</name>" \
"      <dataType>string</dataType>" \
"      <allowedValueList>" \
"        <allowedValue>BrowseMetadata</allowedValue>" \
"        <allowedValue>BrowseDirectChildren</allowedValue>" \
"      </allowedValueList>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Filter</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_SortCriteria</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Index</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_Count</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_UpdateID</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TransferID</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TransferStatus</name>" \
"      <dataType>string</dataType>" \
"      <allowedValueList>" \
"        <allowedValue>COMPLETED</allowedValue>" \
"        <allowedValue>ERROR</allowedValue>" \
"        <allowedValue>IN_PROGRESS</allowedValue>" \
"        <allowedValue>STOPPED</allowedValue>" \
"      </allowedValueList>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TransferLength</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TransferTotal</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_TagValueList</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>A_ARG_TYPE_URI</name>" \
"      <dataType>uri</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>SearchCapabilities</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"no\">" \
"      <name>SortCapabilities</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"yes\">" \
"      <name>SystemUpdateID</name>" \
"      <dataType>ui4</dataType>" \
"    </stateVariable>" \
"    <stateVariable sendEvents=\"yes\">" \
"      <name>ContainerUpdateIDs</name>" \
"      <dataType>string</dataType>" \
"    </stateVariable>" \
"  </serviceStateTable>" \
"</scpd>"

#define CDS_DESCRIPTION_LEN strlen (CDS_DESCRIPTION)

#define CDS_LOCATION "/services/cds.xml"

#define CDS_SERVICE_ID   "urn:upnp-org:serviceId:ContentDirectory"
#define CDS_SERVICE_TYPE "urn:schemas-upnp-org:service:ContentDirectory:1"

#define CDS_URL              "cds.xml"
#define CDS_CONTROL_URL      "cds_control"
#define CDS_EVENT_URL        "cds_event"

#endif /* CDS_H */
