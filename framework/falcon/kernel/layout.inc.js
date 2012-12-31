/* -----------------------------------------------------------------------------
 * Native Object Model Framework (@) 2013 Stight.com                           * 
 * ----------------------------------------------------------------------------- 
 * Version: 	1.0
 * Author:		Vincent Fontaine
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 *
 */

"use strict";

var DOMElement = function(type, options, parent){
	var o = this.options = options || {},
		p = this.parent = parent ? parent : null; // parent element

	if (!Native.elements[type]) {
		throw("Undefined element " + type);
	}

	if (o && o.class) {
		o.className = o.class;
	}

	this.parent = p;
	this.nodes = {}; // children elements

	/* Read Only Properties */
	DOMElement.defineReadOnlyProperties(this, {
		type : type
	});

	/* Public Properties (visual impact on element, need redraw) */
	/* Common to all elements */
	DOMElement.definePublicProperties(this, {
		left : OptionalNumber(o.left, 0),
		top : OptionalNumber(o.top, 0),
		width : o.width ? Number(o.width) : p ? p._width : window.width,
		height : o.height ? Number(o.height) : p ? p._height : window.height,

		offsetLeft : OptionalNumber(o.offsetLeft, 0),
		offsetTop : OptionalNumber(o.offsetTop, 0),

		paddingLeft : OptionalNumber(o.paddingLeft, 0),
		paddingRight : OptionalNumber(o.paddingLeft, 0),

		/* text related properties */
		label : OptionalString(o.label, ""),
		fontSize : OptionalNumber(o.fontSize, 12),
		fontType : OptionalString(o.fontType, "arial"),
		textAlign : OptionalAlign(o.textAlign, "left"),
		lineHeight : OptionalNumber(o.lineHeight, 18),

		// -- misc properties
		blur : OptionalNumber(o.blur, 0),
		opacity : OptionalNumber(o.opacity, 1),

		shadowOffsetX : OptionalNumber(o.shadowOffsetX, 0),
		shadowOffsetY : OptionalNumber(o.shadowOffsetY, 0),
		shadowBlur : OptionalNumber(o.shadowBlur, 0),
		shadowColor : OptionalNumber(o.shadowColor, "rgba(0, 0, 0, 0.5)"),

		color : OptionalValue(o.color, ''),
		background : OptionalValue(o.background, ''),
		backgroundImage : OptionalValue(o.backgroundImage, ''),
		radius : OptionalNumber(o.radius, 0, 0),

		// class management
		className : OptionalString(o.class, ""),

		// -- misc flags
		canReceiveFocus : OptionalBoolean(o.canReceiveFocus, false),
		outlineOnFocus : OptionalBoolean(o.outlineOnFocus, false),

		visible : OptionalBoolean(o.visible, true),
		selected : OptionalBoolean(o.selected, false),
		overflow : OptionalBoolean(o.overflow, true),
		fixed : OptionalBoolean(o.fixed, false),

		hover : false,
		hasFocus : false
	});

	/* Internal Hidden Properties */
	DOMElement.defineInternalProperties(this, {
		private : {},
		_root : p ? p._root : this,
		_nid : Native.layout.objID++,
		_uid : "_obj_" + Native.layout.objID,
		_eventQueues : [],
		_mutex : [],
		_locked : true,

		_absx : 0,
		_absy : 0,

		_layerPadding : 10,
		_cachedBackgroundImage : null,

		_needRefresh : true,
		_needRedraw : true,
		_needPositionUpdate : true,
		_needSizeUpdate : true,
		_needOpacityUpdate : true
	});

	/* Runtime changes does not impact the visual aspect of the element */
	this.id = OptionalString(o.id, this._uid);
	this.name = OptionalString(o.name, "");

	this.isOnTop = false;
	this.mouseOverPath = false;

	Native.elements.init(this);

	if (this.className != '') {
		Native.StyleSheet.set(this);
	}
};

Native.proxy = {
	__noSuchMethod__ : function(id, args){
		throw("Undefined method " + id);
	},

	__lock : function(){
		this._locked = true;
	},

	__unlock : function(){
		this._locked = false;
	},

	add : function(type, options){
		var element = new DOMElement(type, options, this);
		this.addChild(element);
		return element;
	},

	remove : function(){
		Native.layout.remove(this);
		Native.layout.update();
	},

	show : function(){
		if (!this.visible) {
			this.visible = true;
		}
		return this;
	},

	hide : function(){
		if (this.visible) {
			this.visible = false;
		}
		return this;
	},

	focus : function(){
		return this;
	},

	addChild : function(element){
		this.nodes[element._uid] = element;
		element.parent = this;
		element.parent.layer.add(element.layer);
		Native.layout.update();
	},

	removeChild : function(element){
		if (element.parent != this) {
			throw("Unable to remove this element.");
		}
		Native.layout.remove(element);
		Native.layout.update();
	},

	getLayerPixelWidth : function(){
		return Math.round(this._width + 2*this._layerPadding);
	},

	getLayerPixelHeight : function(){
		return Math.round(this._height + 2*this._layerPadding);
	},

	/*
	 * Sort DOMElements to match hardware physical layers order.
	 */
	resetNodes : function(){
		if (!this.parent) return false;

		var parent = this.parent, // parent of this virtual element
			layers = parent.layer.getChildren(); // physical children

		/* Reset parent's nodes */
		parent.nodes = {};

		/* reconstruct the nodes in the right order */
		for (var i in layers){
			// get the host element (that's it : the virtual element)
			var element = layers[i].host;

			// add the element in parent's nodes 
			parent.nodes[element._uid] = element;
		}

		Native.layout.update();
	},

	bringToFront : function(){
		this.layer.bringToFront();
		this.resetNodes();
		return this;
	},

	sendToBack : function(){
		this.layer.sendToBack();
		this.resetNodes();
		return this;
	},

	refresh : function(){
		var p = this.parent,
			x = this._left + this._offsetLeft,
			y = this._top + this._offsetTop;

		/*
		this._absx = p ? p._absx + x : x;
		this._absy = p ? p._absy + y : y;
		*/

		this.__layerPadding = p ? p._layerPadding - this._layerPadding 
								: this._layerPadding;

		if (this.layer) {

			this.layer.visible = this._visible;
			
			if (this._needOpacityUpdate){
				this.layer.context.globalAlpha = this._opacity;
				this._needOpacityUpdate = false;
			}

			if (this._needPositionUpdate){
				this.layer.left = Math.round(this._left + this.__layerPadding);
				this.layer.top = Math.round(this._top + this.__layerPadding);
				this._needPositionUpdate = false;
			}

			if (this._needSizeUpdate){
				var w = this.getLayerPixelWidth(),
					h = this.getLayerPixelHeight();
				this.layer.width = w;
				this.layer.height = h;
				this._needSizeUpdate = false;
			}

			if (this._needRedraw) {
				this.layer.clear();
				this.draw(this.layer.context);
				this._needRedraw = false;
			}
		}
		this._needRefresh = false;
	},

	getDrawingBounds : function(){
		var p = this.parent;
		return {
			x : 0 + this.offsetLeft + this._layerPadding,
			y : 0 + this.offsetTop + this._layerPadding,
			w : this.width,
			h : this.height
		};
	},

	beforeDraw : function(){

	},

	afterDraw : function(){

	},

	isPointInside : function(mx, my){
		this._absx = Math.round(this.layer.__left + this._layerPadding);
		this._absy = Math.round(this.layer.__top + this._layerPadding);

		var x1 = this._absx+1,
			y1 = this._absy+2,
			x2 = x1 + this._width,
			y2 = y1 + this._height;

		return (mx>=x1 && mx<x2 && my>=y1 && my<y2) ? true : false;
	},

	isVisible : function(){
		return this.visible;
	},

	hasClass : function(name){
		return new RegExp('(\\s|^)'+name+'(\\s|$)').test(this.className);
	},

	addClass : function(name){
		if (!this.hasClass(name)){
			this.className += (this.className ? ' ' : '') + name;
		}
		return this;
	},

	setClass : function(name){
		this.className = name;
		return this;
	},

	removeClass : function(name){
		if (this.hasClass(name)){
			let r = new RegExp('(\\s|^)'+name+'(\\s|$)'),
				k = this.className;

			this.className = k.replace(r,' ').replace(/^\s+|\s+$/g, '');
		}
		return this;
	},

	setProperties : function(options){
		for (var key in options){
			if (options.hasOwnProperty(key)){
				this[key] = options[key];
			}
		}
		return this;
	}
};

DOMElement.prototype = {
	__noSuchMethod__ : Native.proxy.__noSuchMethod__,

	__lock : Native.proxy.__lock, // disable setter events
	__unlock : Native.proxy.__unlock, // enable setter events

	add : Native.proxy.add,
	remove : Native.proxy.remove,
	show : Native.proxy.show,
	hide : Native.proxy.hide,
	focus : Native.proxy.focus,

	addChild : Native.proxy.addChild,
	removeChild : Native.proxy.removeChild,

	refresh : Native.proxy.refresh,
	
	beforeDraw : Native.proxy.beforeDraw,
	afterDraw : Native.proxy.afterDraw,

	isPointInside : Native.proxy.isPointInside,
	isVisible : Native.proxy.isVisible,
	getDrawingBounds : Native.proxy.getDrawingBounds,

	hasClass : Native.proxy.hasClass,
	addClass : Native.proxy.addClass,
	removeClass : Native.proxy.removeClass,

	getLayerPixelWidth : Native.proxy.getLayerPixelWidth,
	getLayerPixelHeight : Native.proxy.getLayerPixelHeight,

	bringToFront : Native.proxy.bringToFront,
	sendToBack : Native.proxy.sendToBack,
	resetNodes : Native.proxy.resetNodes,

	setProperties : Native.proxy.setProperties,

	/* user customisable methods */
	update : function(context){},
	draw : function(context){}
};

DOMElement.firePropertyUpdateEvent = function(e){
	var element = e.element,
		old = e.oldValue,
		value = e.newValue;

	element.__lock();

	element.fireEvent("change", {
		property : e.property,
		oldValue : e.oldValue,
		newValue : e.newValue
	});

	switch (e.property) {
		case "left" :
		case "top" :
			element._needPositionUpdate = true;
			break;

		case "width" :
		case "height" :
			element._needSizeUpdate = true;
			break;

		case "opacity" :
			element._needOpacityUpdate = true;
			element._needRedraw = true;
			break;

		case "className" :
			Native.StyleSheet.set(element);
			element._needRedraw = true;
			break;

		default :
			element._needRedraw = true;
			break
	};

	element._needRefresh = true;
	element.__unlock();

};

DOMElement.defineNativeProperty = function(descriptor){
	var element = descriptor.element, // target element
		property = descriptor.property, // property
		value = OptionalValue(descriptor.value, null), // default value
		setter = OptionalCallback(descriptor.setter, null),
		getter = OptionalCallback(descriptor.getter, null),
		writable = OptionalBoolean(descriptor.writable, true),
		enumerable = OptionalBoolean(descriptor.enumerable, true),
		configurable = OptionalBoolean(descriptor.configurable, false);

	if (!element || !property) return false;

	/* if value is undefined, get the previous value */
	if (value == undefined && element["_"+property] && element[property]) {
		value = element["_"+property];
	}

	/* define mirror hidden properties */
	Object.defineProperty(element, "_"+property, {
		value : value,
		enumerable : false,
		writable : true,
		configurable : false
	});

	/* define public accessor */
	Object.defineProperty(element, property, {
		get : function(){
			var r = undefined;
			if (element._locked === false) {
				element.__lock();
				r = getter ? getter.call(element) : undefined;
				element.__unlock();
			}
			return r == undefined ? element["_"+property] : r;
		},

		set : function(newValue){
			var oldValue = element["_"+property];

			if (newValue === oldValue || writable === false) {
				return false;
			} else {
				element["_"+property] = newValue;
			}

			if (element.loaded && element._locked === false) {
				/* lock element */
				element.__lock();

				/* fire propertyupdate event if needed */
				DOMElement.firePropertyUpdateEvent({
					element : element,
					property : property,
					oldValue : oldValue,
					newValue : newValue
				});

				/* optional user defined setter method */
				if (setter){
					var r = setter.call(element, newValue);
					if (r === false) {
						// handle readonly, restore old value
						element["_"+property] = oldValue;
						return false;
					}
				}

				/* unlock element */
				element.__unlock();

			}

		},

		enumerable : enumerable,
		configurable : configurable
	});
};

DOMElement.defineDescriptors = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			var descriptor = props[key],
				value = undefined;

			if (descriptor.value){
				if (typeof descriptor.value == "function"){
					value = descriptor.value.call(element);
				} else {
					value = descriptor.value;
				}
			}

			if (descriptor.writable === false){
				if (descriptor.setter || descriptor.getter){
					throw(element.type + '.js : Setter and Getter must not be defined for non writabe property "'+key+'".');
				}			
			}

			this.defineNativeProperty({
				element : element,
				property : key,
				value : value,

				setter : descriptor.set,
				getter : descriptor.get,

				writable : descriptor.writable,
				enumerable : descriptor.enumerable,
				configurable : descriptor.configurable
			});
		}
	}
};

DOMElement.definePublicProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			this.defineNativeProperty({
				element : element,
				property : key,
				value : props[key],
				configurable : true
			});
		}
	}
};

DOMElement.defineReadOnlyProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			Object.defineProperty(element, key, {
				value : props[key],
				enumerable : true,
				writable : false,
				configurable : false
			});
		}
	}
};

DOMElement.defineInternalProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			Object.defineProperty(element, key, {
				value : props[key],
				enumerable : false,
				writable : true,
				configurable : false
			});
		}
	}
};

DOMElement.implement = function(props){
	Object.merge(DOMElement.prototype, props);
};

var Application = function(options){
	options = options || {};
	//options.background = OptionalValue(options.background, '#262722');
	options.canReceiveFocus = true;
	options.outlineOnFocus = false;

	var element = new DOMElement("UIView", options, null);
	Native.layout.update();

	return element;
};

window.requestAnimationFrame(function(){
	Native.FPS.start();
	Native.layout.draw();
	Native.FPS.show();
});


