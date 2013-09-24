/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

"use strict";

/* -------------------------------------------------------------------------- */

window.events = {
	options : {
		pointerHoldTime : 1500,
		doubleClickInterval : 250
	},

	last : null,
	timer : null,

	mousedown : false,
	mousehold : false,

	preventmouseclick : false,
	preventdefault : false,

	dragging : false,
	dragstarted : false,

	cloneElement : null,	
	sourceElement : null,

	__dataTransfer : {
		data : {},

		set : function(o){
			for (var i in o){
				if (o.hasOwnProperty(i)){
					this.data[i] = o[i];
				}
			}
		},

		setData : function(key, value){
			this.data[key] = value;
		},

		getData : function(key){
			return this.data[key];
		},

		effectAllowed : function(){

		},

		reset : function(){
			delete(this.data);
			this.data = {};
		}
	},

	stopDrag : function(){
		this.mousedown = false;
		this.dragging = false;
		this.dragstarted = false;
		this.sourceElement = null;
	},

	setSource : function(e, source, target){
		e.source = source;
		e.target = target;
	},

	hook : function(element, e){
		if (typeof window.onElementUnderPointer == "function"){
			window.onElementUnderPointer.call(element, e);
		}
	},

	tick : function(){
		this.dispatch("tick", {
			x : window.mouseX,
			y : window.mouseY,
			xrel : 0,
			yrel : 0
		});
	},

	dispatch : function(name, e){
		var self = this,
			x = e.x,
			y = e.y,

			cancelBubble = false,
			cancelEvent = false,

			z = document.getElements();

		e.preventDefault = function(){
			self.preventdefault = true;
		};

		e.preventMouseEvents = function(){
			self.preventmouseclick = true;
		};

		e.cancelBubble = function(){
			cancelBubble = true;
		};

		e.stopPropagation = function(){
			cancelBubble = true;
			cancelEvent = true;
		};

		e.forcePropagation = function(){
			cancelBubble = false;
			cancelEvent = false;
		};

		var __mostTopElementHooked = false;

		if (name == "mousedown" || name == "mouseup") {
			if (this.preventmouseclick) {
				this.preventmouseclick = false;
				return;
			}
		}

		Native.layout.topElement = false;

		for(var i=z.length-1 ; i>=0 ; i--) {
			var element = z[i];
			cancelEvent = false;

			if (!element.layer.__visible) {
				continue;
			}

			if (this.preventdefault && element == document) {
				this.preventdefault = false;
				continue;
			}

			if (name=='keydown'){
				if (e.keyCode == 1073742051 || e.keyCode == 1073742055) {
					element.cmdKeyDown = true;
				}
				if (e.keyCode == 1073742049 || e.keyCode == 1073742053) {
					element.shiftKeyDown = true;
				}
			}

			if (name=='keyup'){
				if (e.keyCode == 1073742051 || e.keyCode == 1073742055) {
					element.cmdKeyDown = false;
				}
				if (e.keyCode == 1073742049 || e.keyCode == 1073742053) {
					element.shiftKeyDown = false;
				}
			}

			e.shiftKeyDown = element.shiftKeyDown == undefined ?
							false : element.shiftKeyDown;

			e.cmdKeyDown = element.cmdKeyDown == undefined ?
							false : element.cmdKeyDown;

			if (name=='keydown' || name=='keyup' || name=='textinput'){
				if (element.canReceiveKeyboardEvents){
					element.fireEvent(name, e);
				}
			} else if (element.isPointInside(x, y)){

				if (Native.layout.topElement === false) {
					Native.layout.topElement = element;
				}

				if (__mostTopElementHooked === false){
					if (element._background || element._backgroundImage){
						self.hook(element, e);
						this.mostTopElementUnderMouse = element;
						__mostTopElementHooked = true;
					}
				}

				switch (name) {
					case "contextmenu" :
						e.element = Native.layout.topElement;
						break;

					case "mousewheel" :
						//cancelBubble = true;
						break;

					case "tick" :
						this.setSource(e, this.sourceElement, element);
						cancelBubble = this.fireMouseOver(element, e);
						break;

					case "mousemove" :
						this.setSource(e, this.sourceElement, element);
						cancelBubble = this.fireMouseOver(element, e);
						break;

					case "drag" :
						cancelEvent = true;
						window.cursor = "drag";
						if (!e.source) {
							this.stopDrag();
						}
						break;

					case "dragover" :
						this.setSource(e, this.sourceElement, element);

						if (!e.source) {
							cancelEvent = true;
							this.stopDrag();
						}

						cancelBubble = this.fireMouseOver(element, e);
						break;

					case "dragstart" :
						e.source = element;
						e.target = element;
						e.source.dragendFired = false;

						this.sourceElement = element;

						if (element.draggable) {
							this.cloneElement = element.clone();
						}
						cancelBubble = true;
						break;

					case "drop":
						this.setSource(e, this.sourceElement, element);
						this.cursor = element.cursor;

						if (!e.source.dragendFired){
							e.source.dragendFired = true;
							e.source.fireEvent("dragend", e);
						}
						break;

					default : break;
				}
				
				if (cancelEvent===false){
					element.fireEvent(name, e);
				}

			} else {
				e.source = this.sourceElement || element;
				e.target = element;
				cancelBubble = this.fireMouseOut(element, e);
			}

			if (cancelBubble) break;
		}

		if (name=="drag"){
			e.source = this.sourceElement;
			if (e.source) {
				e.source.fireEvent("drag", e);
			}
		}
	},

	/* -- VIRTUAL EVENTS PROCESSING ----------------------------------------- */

	fireMouseOver : function(element, e){
		if (!element.mouseover) {
			element.mouseover = true;
			element.mouseout = false;
			if (this.dragging) {
				element.fireEvent("dragenter", e);
			} else {
				element.fireEvent("mouseover", e);
			}
			return false;
		}
		
		if (this.mostTopElementUnderMouse) {
			window.cursor = this.mostTopElementUnderMouse.disabled ?
					"arrow" : this.mostTopElementUnderMouse.cursor;
		}
	},

	fireMouseOut : function(element, e){
		if (element.mouseover && !element.mouseout) {
			element.mouseover = false;
			element.mouseout = true;
			if (this.dragging) {
				element.fireEvent("mouseout", e);
				element.fireEvent("mouseleave", e);
				element.fireEvent("dragleave", e);
				return false;
			} else {
				element.fireEvent("mouseout", e);
				element.fireEvent("mouseleave", e);
				return true;
			}
		}
	},

	/* -- PHYSICAL EVENTS PROCESSING ---------------------------------------- */

	mousedownEvent : function(e){
		var self = this,
			o = this.last || {x:0, y:0},
			dist = Math.distance(o.x, o.y, e.x, e.y) || 0;

		window.mouseX = e.x;
		window.mouseY = e.y;

		if (e.which == 3) {
			this.preventmouseclick = false;
			this.dispatch("contextmenu", e);
		}
		this.mousedown = true;
		this.dragstarted = false;

		e.time = +new Date();
		e.duration = null;
		e.dataTransfer = this.__dataTransfer;

		this.dispatch("mousedown", e);

		this.timer = window.timer(function(){
			self.dispatch("mouseholdstart", e);
			self.mousehold = true;
			this.remove();
		}, this.options.pointerHoldTime);

		if (this.last){
			e.duration = e.time - this.last.time;

			if (dist<4 && e.duration <= this.options.doubleClickInterval) {
				this.dispatch("mousedblclick", e);
			}
		}

		this.last = e;
	},

	mousemoveEvent : function(e){
		window.mouseX = e.x;
		window.mouseY = e.y;
		e.dataTransfer = this.__dataTransfer;

		if (this.mousedown){

			this.timer && this.timer.remove();

			if (this.dragstarted){
				this.dragging = true;
				this.dispatch("dragover", e);
				this.dispatch("drag", e);

				let (ghost = this.cloneElement){
					if (ghost) {
						ghost.left += e.xrel;
						ghost.top += e.yrel;
					}
				}

			} else {
				this.dragstarted = true;
				this.dispatch("dragstart", e);
			}

		} else {
			this.dispatch("mousemove", e);
		}
	},

	mouseupEvent : function(e){
		var self = this,
			o = this.last || {x:0, y:0},
			dist = Math.distance(o.x, o.y, e.x, e.y) || 0;

		// mouseup can be fired without a previous mousedown
		// this prevent mouseup to be fired after (MouseDown + Refresh)
		if (this.mousedown === false) {
			return false;
		}

		this.mousedown = false;
		this.timer && this.timer.remove();

		e.dataTransfer = this.__dataTransfer;

		if (this.dragstarted && this.sourceElement){
			if (this.cloneElement){
				this.cloneElement.remove();
				delete(this.cloneElement);
			}

			this.dispatch("drop", e);
			if (this.cursor) {
				window.cursor = this.cursor;
			}
		}
		this.dragging = false;
		this.dragstarted = false;
		this.sourceElement = null;

		this.dispatch("mouseup", e);

		if (this.mousehold) {
			this.dispatch("mouseholdend", e);
			this.mousehold = false;
		}

		if (o && dist<5) {
			this.dispatch("mouseclick", e);
		}
	},

	mousewheelEvent : function(e){
		this.dispatch("mousewheel", e);
	},

	keydownEvent : function(e){
		this.dispatch("keydown", e);

		if (e.keyCode == 9) {
			Native.layout.focusNextElement();
		}
		window.keydown = e.keyCode;
	},

	keyupEvent : function(e){
		this.dispatch("keyup", e);
		window.keydown = null;
	},

	textinputEvent : function(e){
		e.text = e.val;
		this.dispatch("textinput", e);
	}

};

/* -- DOM HELPERS IMPLEMENTATION -------------------------------------------- */

NDMElement.implement({

	mouseover : false,
	mouseout : false,
	dragendFired : false,

	click : function(cb){
		if (typeof cb == "function") {
			this.addEventListener("mouseclick", cb, false);
		} else {
			this.fireEvent("mouseclick", {
				x : window.mouseX,
				y : window.mouseY
			});
		}
		return this;
	},

	hoverize : function(cbOver, cbOut){
		if (typeof cbOver == "function") {
			this.addEventListener("mouseover", cbOver, false);
		}
		if (typeof cbOut == "function") {
			this.addEventListener("mouseout", cbOut, false);
		}
		return this;
	},

	drag : function(cb){
		if (typeof cb == "function") {
			this.addEventListener("drag", cb, false);
		}
		return this;
	}
});

/* -- DOM EVENTS IMPLEMENTATION --------------------------------------------- */

Object.attachEventSystem = function(proto){
	proto.addEventListener = function(name, callback, propagation){
		var self = this;
		self._eventQueues = self._eventQueues ? self._eventQueues : [];
		var queue = self._eventQueues[name];

		queue = !queue ? self._eventQueues[name] = [] : 
						 self._eventQueues[name];

		queue.push({
			name : OptionalString(name, "error"),
			fn : OptionalCallback(callback, function(){}),
			propagation : OptionalBoolean(propagation, true),
			response : null
		});

		self["on"+name] = function(e){
			for(var i=queue.length-1; i>=0; i--){
				queue[i].response = queue[i].fn.call(self, e);
				if (!queue[i].propagation){
					continue;
				}
			}
		};
		return this;
	};

	proto.fireEvent = function(name, e, successCallback){
		var acceptedEvent = true,
			listenerResponse = true,
			cb = OptionalCallback(successCallback, null);

		if (typeof this["on"+name] == 'function'){
			if (e !== undefined){
				e.dx = e.xrel;
				e.dy = e.yrel;
				e.refuse = function(){
					acceptedEvent = false;
				};
				listenerResponse = this["on"+name](e);
				if (cb && acceptedEvent) cb.call(this);

				return OptionalBoolean(listenerResponse, true);
			} else {
				listenerResponse = this["on"+name]();
			}
		} else {
			if (cb) cb.call(this);
		}
		return this;
	};
};

Object.attachEventSystem(NDMElement.prototype);
Object.attachEventSystem(Thread.prototype);
Object.attachEventSystem(window);


/* -- MOUSE EVENTS ---------------------------------------------------------- */

window._onmousedown = function(e){
	window.events.mousedownEvent(e);
	window.onmousedown(e);
};

window._onmousemove = function(e){
	window.events.mousemoveEvent(e);
	window.onmousemove(e);
};

window._onmousewheel = function(e){
	window.events.mousewheelEvent(e);
	window.onmousewheel(e);
};

window._onmouseup = function(e){
	window.events.mouseupEvent(e);
	window.onmouseup(e);
};

/* -- KEYBOARD EVENTS ------------------------------------------------------- */

window._onkeydown = function(e){
	window.events.keydownEvent(e);
	window.onkeydown(e);
};

window._onkeyup = function(e){
	window.events.keyupEvent(e);
	window.onkeyup(e);
};

window._ontextinput = function(e){
	window.events.textinputEvent(e);
	window.ontextinput(e);
};

/* -- WINDOW EVENTS --------------------------------------------------------- */

window._onfocus = function(e){
	window.onfocus(e);
};

window._onblur = function(e){
	window.onblur(e);
};

/* -- LOAD EVENTS ----------------------------------------------------------- */

window._onready = function(LST){
	Native.core.onready();
	window.onload();
	NDMLayoutParser.parse(LST, function(){
		window.onready();
		document.fireEvent("DOMContentLoaded", {});
	});
};

window._onassetready = function(e){
	switch (e.tag) {
		case "style" :
			Native.StyleSheet.refresh();
			break;
		default : break
	}
};

/* -- USER LAND EVENTS ------------------------------------------------------ */

window.onload = function(){};
window.onready = function(){};
window.onmousedown = function(e){};
window.onmousemove = function(e){};
window.onmousewheel = function(e){};
window.onmouseup = function(e){};
window.onkeydown = function(e){};
window.onkeyup = function(e){};
window.ontextinput = function(e){};
window.onfocus = function(e){};
window.onblur = function(e){};
