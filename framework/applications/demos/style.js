/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 Stight.com - Vincent Fontaine */
/* -------------------------------------- */

"use strict";

/* ---------------------------------------------------------------------------*/
Native.StyleSheet.load("applications/demos/style.nss"); // not blocking
/* ---------------------------------------------------------------------------*/


//document.addEventListener("load", function(){

	var body = new Application();
	body.className = "blue";

	var label = new UILabel(body, {
		left : 10,
		top : 10,
		label : "Native Style Sheet Demo",
		class : "label"
	});

	var	bb = new UIButton(body, "button doit dark");

	var	b1 = new UIButton(body, {left:10, class:"button demo blue"}),
		b2 = new UIButton(body, {left:62, class:"button demo blue"}),
		b3 = new UIButton(body, {left:114, class:"button demo blue"});

	bb.addEventListener("mousedown", function(e){
		if (bb.toggle) {
			Native.layout.getElementsByClassName("rose").each(function(){
				this.removeClass("rose");
				this.addClass("blue");
			});
			bb.toggle = false;
		} else {
			Native.layout.getElementsByClassName("blue").each(function(){
				this.removeClass("blue");
				this.addClass("rose");
			});
			bb.toggle = true;
		}
	});

//});

