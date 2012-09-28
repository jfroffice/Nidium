/* -------------------------- */
/* Native (@) 2012 Stight.com */
/* -------------------------- */

/* --- DEMO APP --------------------------------------------------------- */

		var gdGreen = canvas.createLinearGradient(0, 0, 0, 400);
		gdGreen.addColorStop(0.00,'#44ff77');
		gdGreen.addColorStop(0.60,'#228855');
		gdGreen.addColorStop(1.00,'#ffffff');

		var gdDark = canvas.createLinearGradient(0, 0, 0, 150);
		gdDark.addColorStop(0.00,'#222222');
		gdDark.addColorStop(0.25,'#444444');
		gdDark.addColorStop(0.60,'#222222');
		gdDark.addColorStop(1.00,'#000000');

		var gdBackground = canvas.createLinearGradient(0, 0, 0, window.height);
		gdBackground.addColorStop(0.00,'#444444');
		gdBackground.addColorStop(1.00,'#111111');

var template = "In olden times when wishing still helped one, there lived a king whose daughters were all beautiful; and the youngest was so beautiful that the sun itself, which has seen so much, was astonished whenever it shone in her face. Close by the king's castle lay a great dark forest, and under an old lime-tree in the forest was a well, and when the day was very warm, the king's child went out to the forest and sat down by the fountain; and when she was bored she took a golden ball, and threw it up on high and caught it; and this ball was her favorite plaything. Close by the king's castle lay a great dark forest, and under an old lime-tree in the forest was a well, and when the day was very warm, the king's child went out to the forest and sat down by the fountain; and when she was bored she took a golden ball, and threw it up on high and caught it; and this ball was her favorite plaything. [ɣ] s'écrit g. Quốc ngữ văn bản bằng tiếng Việt.";
	st = [],
	sampleText = '';
for (var t=0; t<2; t++){
	st.push(template);
}
sampleText = st.join('');

/* --------------- */


/*
var wm1 = new WeakMap(),
    wm2 = new WeakMap();
var o1 = {},
    o2 = function(){},
    o3 = window;
 
wm1.set(o1, 37);
wm1.set(o2, "azerty");
*/

var main = new Application({background : '#262722'}),

	myTabs = [
		/* Tab 0 */ {label : "main.js"},
		/* Tab 1 */ {label : "core.js"},
		/* Tab 2 */ {label : "hello.js"},
		/* Tab 3 */ {label : "foo.cpp", selected:true},
		/* Tab 4 */ {label : "class.cpp"},
		/* Tab 5 */ {label : "opengl.cpp", background:"#202a15", color:"#ffffff"},
		/* Tab 6 */ {label : "2.js", background:"#442033", color:"#ffffff"},
		/* Tab 7 */ {label : "rotation.js"},
		/* Tab 8 */ {label : "scale.js"},
		/* Tab 9 */ {label : "native.inc.js"}
	],

	mainTabController = main.add("UITabController", {
		name : "masterTabs",
		tabs : myTabs,
		background : "#191a18"
	});

var	textView = main.add("UIText", {
		x : 733,
		y : 80,
		w : 280,
		h : 568,
		text : sampleText,
		lineHeight : 18,
		fontSize : 13,
		fontType : "arial",
		textAlign : "justify",
		background : "rgba(255, 255, 255, 1)",
		color : "#000000"
	});

var	docButton1 = main.add("UIButton", {x:10, y:110, label:"docButton1", background:"#222222", radius:3, fontSize:14, selected:false}),

	//docButton1 = new UIButton({x:10, y:110, label:"docButton1", background:"#222222", radius:3, fontSize:14, selected:false});

	docButton2 = main.add("UIButton", {x:10, y:140, label:"docButton2", background:"#4488CC", radius:3, fontSize:13, selected:false}),
	docButton3 = main.add("UIButton", {x:10, y:170, label:"docButton3", background:"#CC4488", radius:6, fontSize:12, selected:false}),
	docButton4 = main.add("UIButton", {x:10, y:200, label:"docButton4", background:"#8844CC", radius:6, fontSize:11, selected:false}),
	docButton5 = main.add("UIButton", {x:10, y:230, label:"docButton5", background:"#4400CC", radius:6, fontSize:10, selected:false}),
	docButton6 = main.add("UIButton", {x:10, y:260, label:"docButton6", background:"#0044CC", radius:6, fontSize:9, selected:false}),

	getTextButton = main.add("UIButton", {x:733, y:50, label:"Get Selection", background:"#0044CC", radius:6, fontSize:13, selected:false}),
	cutButton = main.add("UIButton", {x:858, y:50, label:"Cut", background:"#331111", radius:6, fontSize:13, selected:false}),
	copyButton = main.add("UIButton", {x:904, y:50, label:"Copy", background:"#113311", radius:6, fontSize:13, selected:false}),
	pasteButton = main.add("UIButton", {x:960, y:50, label:"Paste", background:"#111133", radius:6, fontSize:13, selected:false}),


	greenView = main.add("UIView", {id:"greenView", x:140, y:480, w:450, h:220, radius:6, background:"#ffffff", shadowBlur:26}),
	overlayView = greenView.add("UIView", {x:90, y:5, w:154, h:210, background:"rgba(0, 0, 0, 0.50)"}),
	davidButton = greenView.add("UIButton", {x:5, y:5, label:"David", background:"#338800"}),
	redViewButton1 = greenView.add("UIButton", {x:5, y:34, label:"RedView 1", background:"#338800", selected:true}),
	redViewButton2 = greenView.add("UIButton", {x:5, y:70, label:"RedView 2", background:"#338800", selected:false}),
	redViewButton3 = greenView.add("UIButton", {x:5, y:120, label:"RedView 3", background:"#338800", selected:false}),

	redViewButton4 = overlayView.add("UIButton", {x:5, y:5, label:"RedView 4", background:"#222222", selected:false}),
	radio1 = overlayView.add("UIRadio", {x:5, y:36, name:"choice", label:"Select this", selected:true}),
	radio2 = overlayView.add("UIRadio", {x:5, y:56, name:"choice", label:"... or this"});


var	tabController = greenView.add("UITabController", {
	name : "helloTabs",
	y : -32,
	tabs : [
		/* Tab 0 */ {label : "main.js", selected:true},
		/* Tab 1 */ {label : "core.js"},
		/* Tab 5 */ {label : "opengl.cpp", background:"#202a15", color:"#ffffff"},
		/* Tab 6 */ {label : "2.js", background:"#442033", color:"#ffffff"},
		/* Tab 7 */ {label : "rotation.js"}
	]
});

/* ------------------------------------------------- */

/*
textView.caret = {
	x1 : 28,
	y1 : 15,
	x2 : 38,
	y2 : 15
};


var t = +new Date(),
	s;
for (var z=0; z<1; z++){
	//s = textView.getTextSelectionFromCaret(textView.caret);
	s = textView.setCaret(658, 10);
}
echo((+new Date()-t));


console.log(s);
console.log(textView.caret);

echo('"' + textView.text.substr(40, 1) + '"');
echo('"' + textView._textMatrix[0].letters[40].char + '"');
echo(textView._textMatrix[0].letters.length);


echo(' ');
echo('"' + textView.text.substr(213, 451) + '"');
*/

var s = textView.setCaret(70, 50);

textView.addEventListener("textselect", function(s){
	//console.log(s);
	//echo('"' + textView.text.substr(s.offset, s.size) + '"');
	//this.select(false);
	//this.setCaret(s.offset, s.size);
	//this.getTextSelectionFromCaret(this.caret);
});

getTextButton.addEventListener("mousedown", function(e){
	echo(">>" + textView.getTextSelection() + "<<");
	echo("");
});

cutButton.addEventListener("mousedown", function(e){
	textView.cut();
});

copyButton.addEventListener("mousedown", function(e){
	textView.copy();
});

pasteButton.addEventListener("mousedown", function(e){
	textView.paste();
});


//textView.cut(3, 5);
//echo(layout.pasteBuffer);

//s = textView.getTextSelectionFromCaret(textView.caret);
//console.log(textView.caret);


var line = overlayView.add("UILine", {x1:20, y1:110, x2:100, y2:180, split:"quadratic", color:"#ff0000"});
var brique = main.add("UIView", {x:150, y:150, w:60, h:60, radius:4, background:"rgba(255,0,0,0.2)", draggable:true});


var win = main.add("UIWindow", {
	x : 280,
	y : 100,
	w : 300,
	h : 200,
	background : "#191a18",
	resizable : true,
	closeable : true,
	movable : true
});

docButton1.addEventListener("mousedown", function(e){
	if (!this.toggle) {
		greenView.bounceScale(0.0, 150, function(){
			this.visible = false;
		}, FXAnimation.easeInOutQuad);
		this.toggle = true;
	} else {
		this.visible = true;
		greenView.bounceScale(2, 300, function(){
			this.scale = 2;
		}, FXAnimation.easeOutElastic);
		this.toggle = false;
	}
});

docButton2.addEventListener("mousedown", function(e){
	redViewButton4.g = {
		x : -redViewButton4.w/2,
		y : -redViewButton4.h/2
	};
	redViewButton4.bounceScale(1.2, 350, function(){});
});

docButton3.addEventListener("mousedown", function(e){
	davidButton.bounceScale(davidButton.scale+1, 150, function(){});
});

docButton4.addEventListener("mousedown", function(e){
	greenView.fadeOut(200, function(){});
});

var blurCache = false;

docButton5.addEventListener("mousedown", function(e){
	canvas.animate = false;

	//if (!blurCache) {
		blurCache = canvas.blur(0, 0, 1024, 768, 2);
	//} else {
		//canvas.putImageData(blurCache, 0, 0);
	//}
});

docButton6.addEventListener("mousedown", function(e){
	canvas.animate = true;
});

//redViewButton4.scale = 2;

greenView.addEventListener("mousedown", function(e){
	this.bringToTop();
	e.stopPropagation();
}, false);

greenView.addEventListener("drag", function(e){
	this.left = e.xrel + this.x;
	this.top = e.yrel + this.y;
});

brique.addEventListener("dragstart", function(e){
	console.log("dragstart : " + e.target.id);
	e.dataTransfer.setData("Text", e.target.id);
});

brique.addEventListener("dragend", function(e){
	console.log("dragend brique");
});

docButton1.addEventListener("dragover", function(e){
	console.log("dragover");
});

docButton1.addEventListener("dragenter", function(e){
	console.log("dragenter " + e.source.id);
});

docButton1.addEventListener("dragleave", function(e){
	console.log("dragleave " + e.source.id);
});

docButton1.addEventListener("drop", function(e){
	console.log("source : " + e.source.id );
	console.log("target : " + e.target.id );

	console.log("using dataTransfer : " + e.dataTransfer.getData("Text") );
});


/*

var file = new DataStream("http://www.google.fr/logo.gif");
file.buffer(4096);
file.chunk(2048);
file.seek(154555);
file.ondata = function(data){
	for (var i=0; i<data.length; i++){
		echo(data[i]);
	}
};
file.open();
file.oncomplete = function(data){};
file.onclose = function(){};

*/

