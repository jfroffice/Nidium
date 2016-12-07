# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

from dokumentor import *

NamespaceDoc("WebSocket Client & Server", """Native implemntation of WebSocket Client & Server
* Client : Connect to a WebSocket server.
* Server : Create a WebSocker server where clients cant connect to.
""")

ClassDoc( "WebSocketServer", "Websocket Server handling.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	section="WebSocket Client & Server"
)

ClassDoc( "WebSocketServerClient", "Class that the Websocket server creates for it's connections.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	section="WebSocket Client & Server"
)

ConstructorDoc( "WebSocketServer", "Constructor for a Websocket listener.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	NO_Examples,
	[ ParamDoc( "host", "hostname or ws:// url", "string", NO_Default, IS_Obligated ),
	 ParamDoc( "protocol", "No-use yet", "string", NO_Default, IS_Optional ) ],
	ReturnDoc( "Websocket instance", "WebSocketServer" )
)

EventDoc( "WebSocketServer.onmessage", "Event triggered when a message arrives.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	NO_Examples,
	[ParamDoc( "client", "The connected client", "WebSocketServerClient", NO_Default, IS_Obligated ),
	ParamDoc( "event", "event object with key data", ObjectDoc([]), NO_Default, IS_Obligated ) ]
)

EventDoc( "WebSocketServer.onopen", "Event triggered when a client connects.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	NO_Examples,
	[ParamDoc( "client", "The connected client", "WebSocketServerClient", NO_Default, IS_Obligated ) ]
)

FunctionDoc("WebSocketServerClient.send", "Send an message over a websocket connection.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	NO_Examples,
	IS_Dynamic, IS_Public, IS_Fast,
	[ParamDoc("message", "Message to be send", "string|ArrayBuffer", NO_Default, IS_Obligated)],
	ReturnDoc("0 on success, or null if failed.", "integer|null")
)

FunctionDoc("WebSocketServerClient.close", "Closes an existing websocket connection.",
	SeesDocs( "WebSocketServerClient|WebSocketServer|WebSocket"),
	NO_Examples,
	IS_Dynamic, IS_Public, IS_Fast,
	NO_Params,
	NO_Returns
)

