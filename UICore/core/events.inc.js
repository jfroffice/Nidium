/* -------------------------- */
/* Native (@) 2012 Stight.com */
/* -------------------------- */

Native.system = {
	doubleClickInterval : 250
};

Native.events = {
	options : {
		pointerHoldTime : 600
	},

	last : null,

	mousedown : false,
	doubleclick : false,

	dragging : false,
	dragstarted : false,

	timer : false,

	cloneElement : null,	
	sourceElement : null,

	__dataTransfer : {
		data : {},

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

	dispatch : function(name, e){
		var x = e.x,
			y = e.y,

			cancelBubble = false,
			cancelEvent = false,

			z = Native.layout.getElements();

		e.stopPropagation = function(){
			cancelBubble = true;
			cancelEvent = true;
		};

		for(var i=z.length-1 ; i>=0 ; i--) {
			var element = z[i];
			cancelEvent = false;

			if (!element.isVisible()) {
				continue;
			}

			if (name=='keydown' || name=='keyup' || name=='textinput'){
				element.fireEvent(name, e);
			}

			if (element.isPointInside(x, y)){

				switch (name) {
					case "mousewheel" :
						cancelBubble = true;
						break;

					case "mousemove" :
						this.setSource(e, this.sourceElement, element);
						cancelBubble = this.fireMouseOver(element, e);
						break;

					case "drag" :
						cancelEvent = true;
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

						if (!e.source.dragendFired){
							e.source.dragendFired = true;
							e.source.fireEvent("dragend", e);
						}
						break;

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
	},

	fireMouseOut : function(element, e){
		if (element.mouseover && !element.mouseout) {
			element.mouseover = false;
			element.mouseout = true;

			if (this.dragging) {
				element.fireEvent("mouseout", e);
				element.fireEvent("mouseleave", e);
				element.fireEvent("ondragleave", e);
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
		this.mousedown = true;
		this.dragstarted = false;

		e.time = +new Date();
		e.duration = null;
		e.dataTransfer = this.__dataTransfer;

		if (this.last){
			e.duration = e.time - this.last.time;
		}

		this.last = e;
		this.dispatch("mousedown", e);

	},

	mousemoveEvent : function(e){
		canvas.mouseX = e.x;
		canvas.mouseY = e.y;
		e.dataTransfer = this.__dataTransfer;

		if (this.mousedown){

			if (this.dragstarted){
				this.dragging = true;
				this.dispatch("dragover", e);
				this.dispatch("drag", e);

				let (ghost = this.cloneElement){
					if (ghost) {
						ghost.left = e.xrel + (ghost.x);
						ghost.top = e.yrel + (ghost.y);
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
			dist = Math.distance(o.x, o.y, e.x, e.y) || 0,
			elapsed = 0;

		// prevent Mouse UP to be fired after (MouseDown + CTRL R)
		if (this.last) {
			elapsed = (+new Date()) - this.last.time;
		}

		this.mousedown = false;

		e.dataTransfer = this.__dataTransfer;

		if (this.dragstarted && this.sourceElement){
			if (this.cloneElement){
				this.cloneElement.remove();
				delete(this.cloneElement);
			}

			this.dispatch("drop", e);
		}
		this.dragging = false;
		this.dragstarted = false;
		this.sourceElement = null;

		//if (!this.timer) {
			this.dispatch("mouseup", e);
		//}

		if (o && dist<3) {

			if (elapsed > this.options.pointerHoldTime) {
				this.doubleclick = false;
				this.timer = false;
				//this.dispatch("mousehold", e);
				this.dispatch("mouseclick", e);
			} else {

				if (!this.timer) {
					setTimer(function(){
						if (!self.doubleclick) {
							self.dispatch("mouseclick", e);
						}
						self.timer = false;
						self.doubleclick = false;
					}, Native.system.doubleClickInterval);

					this.timer = true;
				} else {
					this.dispatch("mousedblclick", e);
					this.doubleclick = true;
					this.timer = false;
				}

			}

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
	},

	keyupEvent : function(e){
		this.dispatch("keyup", e);
	},

	textinputEvent : function(e){
		e.text = e.val;
		this.dispatch("textinput", e);
	}

};

/* -- DOM EVENTS IMPLEMENTATION --------------------------------------------- */

DOMElement.implement({

	mouseover : false,
	mouseout : false,
	dragendFired : false,

	fireEvent : function(name, e){
		canvas.__mustBeDrawn = true;
		if (typeof this["on"+name] == 'function'){
			e.dx = e.xrel / this._scale;
			e.dy = e.yrel / this._scale;
			this["on"+name](e);
		}
	},

	addEventListener : function(name, callback, propagation){
		var self = this,
			queue = self._eventQueues[name];

		queue = !queue ? self._eventQueues[name] = [] : 
						 self._eventQueues[name];

		queue.push({
			name : OptionalString(name, "error"),
			fn : OptionalCallback(callback, function(){}),
			propagation : OptionalBoolean(propagation, true)
		});

		self["on"+name] = function(e){
			for(var i in queue){
				queue[i].fn.call(self, e);
				if (!queue[i].propagation){
					continue;
				}
			}
		};

	}
});

/* -- MOUSE EVENTS ---------------------------------------------------------- */

canvas.onmousedown = function(e){
	Native.events.mousedownEvent(e);
};

canvas.onmousemove = function(e){
	Native.events.mousemoveEvent(e);
};

canvas.onmousewheel = function(e){
	Native.events.mousewheelEvent(e);
};

canvas.onmouseup = function(e){
	Native.events.mouseupEvent(e);
};

/* -- KEYBOARD EVENTS ------------------------------------------------------- */

canvas.onkeydown = function(e){
	Native.events.keydownEvent(e);
};

canvas.onkeyup = function(e){
	Native.events.keyupEvent(e);
};

canvas.ontextinput = function(e){
	Native.events.textinputEvent(e);
};

