/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 Stight.com - Vincent Fontaine */
/* -------------------------------------- */

/*

------
TODO :
------
	- document.location.href = "fdsf/view.nml"; (DONE)
	- contextmenu window.mouseX, window.mouseY (DONE)
	- radial gradient : fail
	- internal file relative to app : no!
	- scale
	- window.resize
	- nml : <viewport>1024x768</viewport>
	- outline

*/

document.background = "#272822";

//load("applications/_tests/timers.js");
//load("applications/_tests/arc.js");
//load("applications/_tests/sockets.client.js");
//load("applications/_tests/sockets.server.js");
//load("applications/_tests/tasks.js");


//load("applications/components/hello.js");
//load("applications/components/motion.js");
//load("applications/components/tabs.js");
//load("applications/components/profiler.js");
//load("applications/components/style.js");
//load("applications/components/windows.js");
load("applications/components/dropdown.js");
//load("applications/components/buttons.js");
//load("applications/components/sliders.js"); // radial gradient issue
//load("applications/components/scrollbars.js");
//load("applications/components/modal.js");
//load("applications/components/threads.js"); // crash
//load("applications/components/tooltips.js");
//load("applications/components/animation.js");
//load("applications/components/flickr.js");
//load("applications/components/http.js");
//load("applications/components/zip.js");

//load("applications/components/text.js"); // FAIL
//load("applications/components/test.js");

/* UNIT TESTS */

	//load("applications/_tests/arc.js");
	//load("testArc.js");
	//load("testRetina.js");


/* MEDIA API */

	//load("applications/audio/test.js"); // CRASH au refresh + BRUIT DE BETE
	//load("applications/audio/mixer.js"); // FAIL TOTAL + BRUIT DE BETE
	//load("applications/audio/dsp.js"); // crash
	//load("applications/media/video.js"); // crash on refresh (and video end)

/* FILE API */

	//load("applications/components/file.basic.js");
	//load("applications/components/file.advanced.js");


/* SHADERS */

	//load("applications/components/shader.js"); // OK
	//load("applications/components/shader.basic.js"); // TODO : relative path to app
	//load("applications/components/shader.advanced.js"); // TODO : relative path to app

/* TUTORIALS */

	//load("applications/tutorials/01.hello.js");
	//load("applications/tutorials/02.buttons.js");
	//load("applications/tutorials/03.events.js");
	//load("applications/tutorials/04.motion.js");
	//load("applications/tutorials/11.post.js");

/* CHARTS DEMOS */

	//load("applications/charts/line.js");
	//load("applications/charts/pie.js");
	//load("applications/charts/polar.js"); // implement vertical text align
	//load("applications/charts/donut.js");
	//load("applications/charts/radar.js");
	//load("applications/charts/bar.js");
	//load("applications/charts/demo.js");

/* Unfinished */

	//load("applications/components/splines.js"); // LENTEUR ABOMINABLE
	//load("applications/components/diagrams.js");



/* -- Native Debugger ------------------ */

//load("applications/NatBug.nap");
