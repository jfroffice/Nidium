# Copyright 2016 Nidium Inc. All rights reserved.
# Use of this source code is governed by a MIT license
# that can be found in the LICENSE file.

from dokumentor import *

NamespaceDoc( "module", "Module that is exported.",
	SeesDocs( "global.modules" ),
        examples=[
            ExampleDoc("module.exports = {\"lorem\":\"ipsum\"}"), 
            ExampleDoc("var foobar = require(\"foobar.js\");\n console.log(foobar.lorem); // Print \"ipsum\"")
        ]
)

FieldDoc( "module.exports", "Exported Module.",
	SeesDocs( "Module|global.modules" ),
	NO_Examples,
	IS_Static, IS_Public, IS_Readonly,
	"mixed",
	NO_Default
)

FieldDoc( "module.id", "The module name.",
	SeesDocs( "Module" ),
	NO_Examples,
	IS_Static, IS_Public, IS_Readonly,
	"integer",
	NO_Default
)

FunctionDoc( "global.require", """`require` implementation similar to NodeJS `require()`""",
	SeesDocs( "Module|Module.name|global.load" ),
	NO_Examples,
	IS_Static, IS_Public, IS_Fast,
	[ParamDoc( "path", "modulename", "string", NO_Default, IS_Obligated ) ],
	ReturnDoc( "modulename", "string" )
)

FieldDoc( "global.modules", "Exposed Modules.",
	SeesDocs( "Module" ),
	NO_Examples,
	IS_Static, IS_Public, IS_Readonly,
	'[Module]',
	NO_Default
)
