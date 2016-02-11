from dokumentor import *

NamespaceDoc("Debugger", """Javascript Debugger for Nidium. The Debugger object is the underlying <a href="https://developer.mozilla.org/en-US/docs/Tools/Debugger-API/Debugger">Debugger of SpiderMonkey </a>.

Nidium is extending SpiderMonkey's Debugger object with two static method to help you create the debugger into another <a href="https://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Compartmentshttps://developer.mozilla.org/en-US/docs/Mozilla/Projects/SpiderMonkey/Compartments">compartement</a> and then run code inside the Debugger compartement""", 
    [ SeeDoc( "Debugger.run" ) ],
    NO_Examples,
)

FunctionDoc("Debugger.create", "Create a new instance of a Debugger into a new compartment",
    [ SeeDoc( "Debugger.run" ) ],
        NO_Examples,
    IS_Static, IS_Public, IS_Fast,
    [],
    ReturnDoc( "Return a DebuggerContext which is an opaque object that olds various information about the context of the newly created Debugger", "DebuggerContext" )
)

FunctionDoc("Debugger.run", """Run a function inside the Debugger compartement. 
        
Important note : You cannot share variables between compartements as you usually do in JS. Each compartement is like a sandbox. If you want to have access to variables from the main compartement, you can pass them as an arguments to the run() method. Nidium will wrap your variables to make them accesible inside the Debugger compartement. 

The compartement of the debugger only expose the console API. None of Nidium APIs will be available.""",
    NO_Sees,
    ExampleDoc( """ var dbgCtx = Debugger.create();
Debugger.run(dbgCtx, function(dbg) {
    // dbg is the Debugger instance
    dbg.onEnterFrame = function(frame) {
        console.log("Entering inside function " + (frame.callee && frame.callee.displayName ? frame.callee.displayName : "anonymous"));
    }
});

// The main compartment is not shared with the Debugger.
// You cannot use variables from the parent scope.
var foo = "bar";
Debugger.run(dbgCtx, function(dbg) {
    console.log(foo); // undefined
});

// However you can explicitly pass a variable into the Debugger compartment
var foo = "bar";
var obj = {"foo": "bar"};
Debugger.run(dbgCtx, function(dbg, wrappedFoo, wrappedObj) {
    console.log(wrappedFoo); // bar
    console.log(JSON.stringify(wrappedObj)); // {"foo": "bar"}
}, foo, obj);
""" ), 
    IS_Static, IS_Public, IS_Fast,
    [
        ParamDoc( 'callback', 'function to be executed in the Debugger compartement', 'function', NO_Default, IS_Obligated ),
        ParamDoc( 'argn', 'Optional variable to wrap into the Debugger compartement. The wrapped variable is passed as an argument of the callback', 'mixed', 0, IS_Optional ) 
    ],
    ReturnDoc( "The value returned from the callback function", "mixed" )
)
