/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

/* -------------------------------------------------------------------------- */
load("libs/misc.lib.js");
/* -------------------------------------------------------------------------- */

/* -- High Level Asynchronous File Access -- */

File.getText("main.js", function(text){
	console.log(text);
});

File.read("main.js", function(buffer, size){
	buffer.dump();
});

File.write("test.txt", "new content", function(){
	File.append("test.txt", " added to this file", function(){
		console.log("append successful");
	});
});
