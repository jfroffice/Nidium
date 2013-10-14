/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

var main = new Application({background:"#222222"});

var ctx = main.add("UICanvas", {
	left : 340,
	top : 500,
	width : 250,
	height : 220,
	background : "#ffffff"
});




/* Create a new 500x500 layer */
var c1 = new Canvas(500, 500),
	ctx1 = c1.getContext("2d");

c1.left = 50;
c1.top = 50;
c1.opacity = 0.9;
ctx1.fillStyle = "green";
ctx1.fillRect(0, 0, 500, 500);

/* attach the layer to its parent (document.layer) */
document.layer.add(c1);

/* Create a new child of layer */
var c2 = new Canvas(250, 250),
	ctx2 = c2.getContext("2d");

c2.left = 400;
c2.top = 400;
c2.opacity = 0.6;
ctx2.fillStyle = "red";
ctx2.fillRect(0, 0, 250, 250);

// attach c2 to c1
c1.add(c2);

setTimeout(function(){
	ctx2.fillStyle = "blue";
	ctx2.fillRect(0, 0, 250, 250);
	c2.left -= 40;
	c2.top -= 40;
	c2.width = 300;
	c2.height = 150;
	c1.left += 40;
}, 1200);
