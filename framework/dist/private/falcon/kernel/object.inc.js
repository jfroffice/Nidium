/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

"use strict";

/* -------------------------------------------------------------------------- */

var NDMElement = function(type, options, parent){
	var o = this.options = options || {},
		p = this.parent = parent ? parent : null; // parent element

	if (!Native.elements[type]) {
		throw("Undefined element " + type);
	}

	if (o && o.class) {
		o.className = o.class;
	}

	this.parent = p;
	this.nodes = []; // children elements

	/* Read Only Properties */
	NDMElement.defineReadOnlyProperties(this, {
		type : type,
		isNDMElement : true
	});

	/* Nidium Engine Properties */
	this.left = OptionalNumber(o.left, 0);
	this.top = OptionalNumber(o.top, 0);

	/*
	this.width = o.width ? Number(o.width) : p ?
					p._width-this.left : window.width-this.left;

	this.height = o.height ? Number(o.height) : p ?
					p._height-this.top : window.height-this.top;
	*/

	/* Public Dynamic Properties (visual impact on element, need redraw) */
	/* Common to all elements */
	NDMElement.definePublicProperties(this, {
		// -- class management
		id : null,
		className : OptionalString(o.class, ""),

		// -- layout properties
		/*
		left : OptionalNumber(o.left, 0),
		top : OptionalNumber(o.top, 0),
		*/
		width : o.width ? Number(o.width) : p ? p._width : window.width,
		height : o.height ? Number(o.height) : p ? p._height : window.height,

		innerWidth : o.innerWidth ? Number(o.innerWidth) : -1,
		innerHeight : o.innerHeight ? Number(o.innerHeight) : -1,

		scrollLeft : OptionalNumber(o.scrollLeft, 0),
		scrollTop : OptionalNumber(o.scrollTop, 0),

		percentLeft : OptionalNumber(o.percentLeft, 0),
		percentTop : OptionalNumber(o.percentTop, 0),
		percentWidth : o.percentWidth ? Number(o.percentWidth) : 100,
		percentHeight : o.percentHeight ? Number(o.percentHeight) : 100,

		offsetLeft : OptionalNumber(o.offsetLeft, 0),
		offsetTop : OptionalNumber(o.offsetTop, 0),

		paddingLeft : OptionalNumber(o.paddingLeft, 0),
		paddingRight : OptionalNumber(o.paddingLeft, 0),
		paddingTop : OptionalNumber(o.paddingTop, 0),
		paddingBottom : OptionalNumber(o.paddingBottom, 0),

		// -- text related properties
		text : OptionalString(o.text, ""),
		label : OptionalString(o.label, ""),
		fontSize : OptionalNumber(o.fontSize, 12),
		fontFamily : OptionalString(o.fontFamily, "arial"),
		textAlign : OptionalAlign(o.textAlign, "left"),
		lineHeight : OptionalNumber(o.lineHeight, 18),
		fontWeight : OptionalWeight(o.fontWeight, "normal"),

		// -- icon related properties
		shape : OptionalString(o.shape, ""),
		variation : OptionalNumber(o.variation, 0),

		// -- style properties
		blur : OptionalNumber(o.blur, 0),
		opacity : OptionalNumber(o.opacity, 1),
		alpha : OptionalNumber(o.alpha, 1),

		shadowBlur : OptionalNumber(o.shadowBlur, 0),
		shadowColor : OptionalValue(o.shadowColor, "rgba(0, 0, 0, 0.5)"),
		shadowOffsetX : OptionalNumber(o.shadowOffsetX, 0),
		shadowOffsetY : OptionalNumber(o.shadowOffsetY, 0),

		textShadowBlur : OptionalNumber(o.textShadowBlur, 0),
		textShadowColor : OptionalValue(o.textShadowColor, ''),
		textShadowOffsetX : OptionalNumber(o.textShadowOffsetX, 0),
		textShadowOffsetY : OptionalNumber(o.textShadowOffsetY, 0),

		color : OptionalValue(o.color, ''),
		background : OptionalValue(o.background, ''),
		backgroundImage : OptionalValue(o.backgroundImage, ''),
		backgroundRepeat : OptionalBoolean(o.backgroundRepeat, true),
		radius : OptionalNumber(o.radius, 0, 0),
		borderWidth : OptionalNumber(o.borderWidth, 0),
		borderColor : OptionalValue(o.borderColor, ''),
		outlineColor : OptionalValue(o.outlineColor, 'blue'),

		angle : OptionalNumber(o.angle, 0),
		scale : OptionalNumber(o.scale, 1),

		// -- misc flags
		canReceiveFocus : OptionalBoolean(o.canReceiveFocus, false),
		canReceiveKeyboardEvents : OptionalBoolean(
			o.canReceiveKeyboardEvents,
			false
		),
		outlineOnFocus : OptionalBoolean(o.outlineOnFocus, true),

		visible : OptionalBoolean(o.visible, true),
		hidden : OptionalBoolean(o.hidden, false),
		selected : OptionalBoolean(o.selected, false),
		overflow : OptionalBoolean(o.overflow, true),
		multiline : OptionalBoolean(o.multiline, false),
		editable : OptionalBoolean(o.editable, false),
		disabled : OptionalBoolean(o.disabled, false),
		outline : OptionalBoolean(o.outline, false),
		
		scrollable : OptionalBoolean(o.scrollable, false),
		scrollBarX : OptionalBoolean(o.scrollBarX, false),
		scrollBarY : OptionalBoolean(o.scrollBarY, false),
		position : OptionalPosition(o.position, "relative"),

		hover : false,
		hasFocus : false,

		cursor : OptionalCursor(o.cursor, "arrow")
	});

	/* Internal Hidden Properties */
	NDMElement.defineInternalProperties(this, {
		private : {},

		flags : 0,
		lineIndex : 0,

		_root : p ? p._root : null,
		_nid : Native.layout.objID++,
		_uid : "_obj_" + Native.layout.objID,
		_eventQueues : [],
		_mutex : [],
		_locked : true,

		_cachedBackgroundImage : null,

		_minx : this._left,
		_miny : this._top,
		_maxx : this._left + this._width,
		_maxy : this._top + this._top,

		_layerPadding : p ? Math.max(this.shadowBlur*5, 20) : Math.max(this.shadowBlur*5, 10),

		/* refreshing flags */
		_needRefresh : true,
		_needRedraw : true,
		_needPositionUpdate : true,
		_needSizeUpdate : true,
		_needOpacityUpdate : true,
		_needAncestorCacheClear : false
	});

	Object.createProtectedHiddenElement(this, "static", {
		default : {}
	});

	/* runtime change of id leads to nss class check */
	this.id = OptionalString(o.id, this._uid);

	/* Runtime changes does not impact the visual aspect of the element */
	this.name = OptionalString(o.name, "");
	this.isOnTop = false;
	this.hasChildren = false;
	this.mouseOverPath = false;

	if (p){
		p.hasChildren = true;
	}
};

/* -------------------------------------------------------------------------- */

NDMElement.prototype = {
	__noSuchMethod__ : Native.object.__noSuchMethod__,

	__lock : Native.object.__lock, // disable setter events
	__unlock : Native.object.__unlock, // enable setter events
	__refresh : Native.object.__refresh, // internal refresh
	__updateLayerOpacity : Native.object.__updateLayerOpacity,
	__updateLayerPosition : Native.object.__updateLayerPosition,
	__updateLayerSize : Native.object.__updateLayerSize,
	__updateAncestors : Native.object.__updateAncestors,
	__resizeLayer : Native.object.__resizeLayer,

	add : Native.object.add,
	remove : Native.object.remove,
	clear : Native.object.clear,
	show : Native.object.show,
	hide : Native.object.hide,
	focus : Native.object.focus,

	addChild : Native.object.addChild,
	appendChild : Native.object.addChild,
	removeChild : Native.object.removeChild,
	insertBefore : Native.object.insertBefore,
	insertAfter : Native.object.insertAfter,
	insertChildAtIndex : Native.object.insertChildAtIndex,

	getChildren : Native.object.getChildren,

	beforeDraw : Native.object.beforeDraw,
	afterDraw : Native.object.afterDraw,

	isPointInside : Native.object.isPointInside,
	isVisible : Native.object.isVisible,
	isAncestor : Native.object.isAncestor,
	isBoundBySelector : Native.object.isBoundBySelector,

	getDrawingBounds : Native.object.getDrawingBounds,
	getBoundingRect : Native.object.getBoundingRect,

	hasClass : Native.object.hasClass,
	addClass : Native.object.addClass,
	removeClass : Native.object.removeClass,

	getPropertyHandler : Native.object.getPropertyHandler,
	applySelectorProperties : Native.object.applySelectorProperties,
	applyStyleSheet : Native.object.applyStyleSheet,

	bringToFront : Native.object.bringToFront,
	sendToBack : Native.object.sendToBack,
	resetNodes : Native.object.resetNodes,

	setProperties : Native.object.setProperties,

	center : Native.object.center,
	centerLeft : Native.object.centerLeft,
	centerTop : Native.object.centerTop,
	move : Native.object.move,
	place : Native.object.place,
	setCoordinates : Native.object.setCoordinates,
	fix : Native.object.fix,

	redraw : Native.object.redraw,

	expand : Native.object.expand,
	shrink : Native.object.shrink,

	/* ----------------------------------------------- */

	set maxWidth(value) {
		var w = value == null || value == '' ? null : Number(value);

		if (w === null) {
			w = this.contentWidth;
			this.width = w;
		} else {
			this.width = Math.min(this._width, w);
		}

		this._maxWidth = w;
	},

	get maxWidth() {
		return this._maxWidth;
	},

	/* ----------------------------------------------- */

	set fastLeft(value) {
		this._left = value;
		this.layer.left = value;
	},

	set fastTop(value) {
		this._top = value;
		this.layer.top = value;
	},

	/* ----------------------------------------------- */
	/* NDMElement.left                                 */
	/* ----------------------------------------------- */

	set left(value) {
		this._left = value;
		if (this.layer) {
			this.layer.left = value;
			this.__updateAncestors();
		}
	},

	get left() {
		return this._left;
	},

	/* ----------------------------------------------- */
	/* NDMElement.top                                  */
	/* ----------------------------------------------- */

	set top(value) {
		this._top = value;
		if (this.layer) {
			this.layer.top = value;
			this.__updateAncestors();
		}
	},

	get top() {
		return this._top;
	},

	/* ----------------------------------------------- */
	/* NDMElement.width                                */
	/* ----------------------------------------------- */
	/*
	set width(value) {
		this._width = value;
		if (this.layer) {
			//this.layer.width = Math.round(value);
			this.__updateAncestors();
			this.redraw();
		}
	},

	get width() {
		return this._width;
	},
	*/
	/* ----------------------------------------------- */
	/* NDMElement.height                               */
	/* ----------------------------------------------- */
	/*
	set height(value) {
		this._height = value;
		if (this.layer) {
			//this.layer.height = Math.round(value);
			this.__updateAncestors();
			this.redraw();
		}
	},

	get height() {
		return this._top;
	},
	*/
	/* ----------------------------------------------- */
	/* READ ONLY WRAPPERS                              */
	/* ----------------------------------------------- */

	get __left() {
		return this.layer.__left;
	},

	get __top() {
		return this.layer.__top;
	},

	get contentWidth() {
		return this.layer ? this.layer.contentWidth : this.width;
	},

	get contentHeight() {
		return this.layer ? this.layer.contentHeight : this.height;
	},

	get children() {
		return this.getChildren();
	},

	get previousSibling() {
		var layer = this.layer.getPrevSibling();
		return layer ? layer.host : null;
	},

	get nextSibling() {
		var layer = this.layer.getNextSibling();
		return layer ? layer.host : null;
	},

	get childNodes() {
		return this.nodes;
	},

	get parentNode() {
		return this.parent ? this.parent : null;
	},

	get nodeType() {
		return this.type;
	},

	get nodeName() {
		return this.name;
	},

	get nodeIndex() {
		return this.parent ? this.parent.nodes.indexOf(this) : -1;
	},

	get ownerDocument() {
		return this._root;
	},

	get hasChildNodes() {
		return this.nodes.length>0;
	},

	get hasOwnerDocument() {
		return this._root !== null;
	},

	get orphaned() {
		return this.parent === null && this._root === null;
	},

	get rooted() {
		return !this.orphaned;
	},

	/* -- Generic Events -- */

	onbeforecopy : null, /* implemented */
	onbeforecut : null, /* implemented */
	onbeforepaste : null, /* implemented */
	oncopy : null, /* implemented */
	oncut : null, /* implemented */
	onpaste : null, /* implemented */
	onselect : null, /* implemented */
	onselectstart : null, /* implemented */

	onblur : null, /* implemented */
	onfocus : null, /* implemented */

	onchange : null, /* implemented */
	oncontextmenu : null, /* implemented */

	ondrag : null, /* implemented */
	ondragend : null, /* implemented */
	ondragenter : null, /* implemented */
	ondragleave : null, /* implemented */
	ondragover : null, /* implemented */
	ondragstart : null, /* implemented */
	ondrop : null, /* implemented */

	onkeydown : null, /* implemented */
	onkeypress : null, /* implemented */
	onkeyup : null, /* implemented */
	ontextinput : null, /* implemented */
	onload : null, /* implemented */

	onmouseclick : null, /* implemented */
	onmousedblclick : null, /* implemented */
	onmousedown : null, /* implemented */
	onmousemove : null, /* implemented */
	onmouseout : null, /* implemented */
	onmouseover : null, /* implemented */
	onmouseup : null, /* implemented */
	onmousewheel : null, /* implemented */

	onreset : null, /* implemented */
	onerror : null, /* implemented */
	onsubmit : null, /* implemented */

	onscroll : null,

	/* -- NSS Properties (to be implemented) -- */

	/*
	backgroundColor : "",
	backgroundImage : "",      ** implemented **
	backgroundPositionX : "",
	backgroundPositionY : "",
	backgroundRepeat : "",

	border : "",      ** implemented **
	borderBottom : "",
	borderBottomColor : "",
	borderBottomLeftRadius : "",
	borderBottomRightRadius : "",
	borderBottomStyle : "",
	borderBottomWidth : "",
	borderCollapse : "",
	borderColor : "",      ** implemented **
	borderImage : "",
	borderImageOutset : "",
	borderImageRepeat : "",
	borderImageSlice : "",
	borderImageSource : "",
	borderImageWidth : "",
	borderLeft : "",
	borderLeftColor : "",
	borderLeftStyle : "",
	borderLeftWidth : "",
	borderRadius : "",
	borderRight : "",
	borderRightColor : "",
	borderRightStyle : "",
	borderRightWidth : "",
	borderSpacing : "",
	borderStyle : "",
	borderTop : "",
	borderTopColor : "",
	borderTopLeftRadius : "",
	borderTopRightRadius : "",
	borderTopStyle : "",
	borderTopWidth : "",
	borderWidth : "",       ** implemented **

	clipPath : "",
	clipRule : "",

	colorInterpolation : "",
	colorInterpolationFilters : "",
	colorRendering : "",

	direction : "",
	display : "",

	fill : "",
	fillOpacity : "",
	fillRule : "",

	filter : "",
	float : "",

	floodColor : "",
	floodOpacity : "",

	font : "",
	fontFamily : "",
	fontSize : "",      ** implemented **
	fontStretch : "",
	fontStyle : "",
	fontVariant : "",
	fontWeight : "",

	imageSmoothingEnabled : "",

	kerning : "",
	letterSpacing : "",
	lightingColor : "",
	lineHeight : "",      ** implemented **

	listStyle : "",
	listStyleImage : "",
	listStylePosition : "",
	listStyleType : "",

	margin : "",
	marginBottom : "",
	marginLeft : "",
	marginRight : "",
	marginTop : "",

	maxHeight : "",
	maxWidth : "",
	minHeight : "",
	minWidth : "",

	outline : "",      ** implemented **
	outlineColor : "",      ** implemented **
	outlineOffset : "",
	outlineStyle : "",
	outlineWidth : "",

	overflow : "",
	overflowWrap : "",
	overflowX : "",
	overflowY : "",

	padding : "",
	paddingBottom : "",
	paddingLeft : "",
	paddingRight : "",
	paddingTop : "",

	strokeDasharray : "",
	strokeDashoffset : "",
	strokeLinecap : "",
	strokeLinejoin : "",
	strokeMiterlimit : "",
	strokeOpacity : "",
	strokeWidth : "",

	tabSize : "",

	textAlign : "",
	textAnchor : "",
	textDecoration : "",
	textIndent : "",
	textLineThrough : "",
	textLineThroughColor : "",
	textLineThroughMode : "",
	textLineThroughStyle : "",
	textLineThroughWidth : "",
	textOverflow : "",
	textOverline : "",
	textOverlineColor : "",
	textOverlineMode : "",
	textOverlineStyle : "",
	textOverlineWidth : "",
	textRendering : "",
	textShadow : "",      ** implemented **
	textTransform : "",
	textUnderline : "",
	textUnderlineColor : "",
	textUnderlineMode : "",
	textUnderlineStyle : "",
	textUnderlineWidth : "",

	verticalAlign : "",
	visibility : "",
	whiteSpace : "",
	zIndex : "",
	zoom : "",
	*/

	/* -- user customisable methods -- */

	onAdoption : function(parent){},
	onAddChildRequest : function(child){},
	onChildReady : function(child){},

	update : function(context){},
	draw : function(context){}
};

/* -------------------------------------------------------------------------- */

NDMElement.onPropertyUpdate = function(e){
	var element = e.element,
		old = e.oldValue,
		value = e.newValue,

		emitter = {
			property : e.property,
			oldValue : e.oldValue,
			newValue : e.newValue
		};

	element.__unlock();

	element.fireEvent("propertyupdate", emitter);

	element.__lock("onPropertyUpdate");

	//element.update.call(element, e.property, e.value);

	switch (e.property) {
		/*
		case "left" :
			element.layer.left = value;
			element.layer.scrollLeft = element._scrollLeft;
			element._needAncestorCacheClear = true;
			break;

		case "top" :
			element.layer.top = value;
			element.layer.scrollTop = element._scrollTop;
			element._needAncestorCacheClear = true;
			break;
		*/
		/*
		case "left" :
		case "top" :
			element._needPositionUpdate = true;
			element._needAncestorCacheClear = true;
			break;
		*/

		case "scrollLeft" :
		case "scrollTop" :
			element._needPositionUpdate = true;
			break;

		case "width" :
			element.layer.width = Math.round(value);
			element._needAncestorCacheClear = true;
			element._needRedraw = true;
			break;

		case "height" :
			element.layer.height = Math.round(value);
			element._needAncestorCacheClear = true;
			element._needRedraw = true;
			break;

		case "opacity" :
			element._needOpacityUpdate = true;
			break;


		case "id" :
		case "disabled" :
		case "hover" :
		case "selected" :
		case "className" :
			element.applyStyleSheet();
			element._needRedraw = true;
			break;

		case "position" :
			break;

		case "angle" :
			element.__resizeLayer();
			break;

		case "cursor" :
			/*
			if (element.isPointInside(window.mouseX, window.mouseY)){
				window.cursor = value;
			}
			*/
			break;

		default :
			element._needRedraw = true;
			break
	};

	element._needRefresh = true;
	element.__refresh();
	element.__unlock("onPropertyUpdate");
};

/* -------------------------------------------------------------------------- */

NDMElement.implement = function(props){
	Object.merge(Native.object, props);
	for (var key in props){
		if (props.hasOwnProperty(key)){
			NDMElement.prototype[key] = Native.object[key];
		}
	}
};

var	isNDMElement = function(element){
	return element && element.isNDMElement;
};

/* -------------------------------------------------------------------------- */

NDMElement.listeners = {
	addDefault : function(element){
		this.addSelectors(element);
		this.addHovers(element);
	},

	addSelectors : function(element){
		element.addEventListener("mousedown", function(e){
			this.selected = true;
		});

		element.addEventListener("mouseup", function(e){
			this.selected = false;
		});

		element.addEventListener("dragend", function(e){
			this.selected = false;
		});
	},

	addHovers : function(element){
		element.addEventListener("mouseover", function(e){
			this.hover = true;
		});

		element.addEventListener("mouseout", function(e){
			this.hover = false;
		});
	}
};

/* -------------------------------------------------------------------------- */

NDMElement.draw = {
	circleBackground : function(element, context, params, radius){
		var gradient = context.createRadialGradient(
			params.x+radius,
			params.y+radius, 
			radius,
			params.x+radius,
			params.y+radius,
			radius/4
		);

		if (element.hover){
			gradient.addColorStop(0.00, 'rgba(0, 0, 0, 0.01)');
			gradient.addColorStop(1.00, 'rgba(0, 0, 0, 0.1)');
		} else {
			gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.01)');
			gradient.addColorStop(1.00, 'rgba(255, 255, 255, 0.1)');
		}

		context.beginPath();
		context.arc(
			params.x+radius, params.y+params.h*0.5, 
			radius, 0, 6.283185307179586, false
		);
		context.setColor(element.background);
		context.fill();
		context.lineWidth = 1;

		context.beginPath();
		context.arc(
			params.x+radius, params.y+params.h*0.5, 
			radius, 0, 6.283185307179586, false
		);
		context.setColor(gradient);
		context.fill();
		context.lineWidth = 1;

		if (element.selected){
			context.beginPath();
			context.arc(
				params.x+radius, params.y+params.h*0.5, 
				radius, 0, 6.283185307179586, false
			);
			context.setColor(element.background);
			context.fill();
			context.lineWidth = 1;
		}
	},

	label : function(element, context, params, color, shadowColor){
		var textOffsetX = element.paddingLeft,
			textOffsetY = (params.h-element.lineHeight)/2 + 4 + element.lineHeight/2;

		var tx = params.x+textOffsetX,
			ty = params.y+textOffsetY;

		if (element.textAlign == "right") {
			tx = params.x + params.w - element._textWidth - element.paddingRight;
		}

		context.fontSize = element.fontSize;
		context.fontFamily = element.fontFamily;

		context.setText(
			element.label,
			params.x+textOffsetX+params.textOffsetX,
			params.y+textOffsetY+params.textOffsetY,
			color ? color : element.color,
			element.textShadowOffsetX,
			element.textShadowOffsetY,
			element.textShadowBlur,
			shadowColor ? shadowColor : element.textShadowColor
		);

	},

	outline : function(element, color){
		var params = element.getDrawingBounds(),
			context = element.layer.context,
			outlineColor = color ? color : element.outlineColor;

		NDMElement.draw.outlineBox(element, context, params, outlineColor);
		NDMElement.draw.outlineBox(element, context, params, outlineColor);
		NDMElement.draw.outlineBox(element, context, params, "rgba(255, 255, 255, 0.8)");
		NDMElement.draw.outlineBox(element, context, params, "rgba(255, 255, 255, 1)");
	},

	outlineBox : function(element, context, params, borderColor){
		context.setShadow(
			0, 0, 4,
			borderColor ? borderColor : "rgba(0, 0, 255, 0.4)"
		);

		context.roundbox(
			params.x-2, params.y-2, 
			params.w+4, params.h+4, 
			element.radius+2,
			"rgba(255, 255, 255, 0.6)",
			null,
			element.borderWidth
		);
		context.setShadow(0, 0, 0);
	},

	box : function(element, context, params, backgroundColor, borderColor){
		context.roundbox(
			params.x, params.y, 
			params.w, params.h, 
			element.radius,
			backgroundColor ? backgroundColor : element.background,
			borderColor ? borderColor : element.borderColor,
			element.borderWidth
		);
	},

	glassLayer : function(element, context, params){
		var gradient = this.getGlassGradient(element, context, params);
		this.box(element, context, params, gradient);
	},

	getGlassGradient : function(element, context, params){
		var gradient = context.createLinearGradient(
			params.x, params.y,
			params.x, params.y+params.h
		);

		if (element.selected){
			gradient.addColorStop(0.00, 'rgba(0, 0, 0, 0.8)');
			gradient.addColorStop(0.25, 'rgba(0, 0, 0, 0.6)');
			gradient.addColorStop(1.00, 'rgba(0, 0, 0, 0.6)');
		} else {
			if (element.hover){
				gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.7)');
				gradient.addColorStop(0.10, 'rgba(255, 255, 255, 0.3)');
				gradient.addColorStop(0.50, 'rgba(255, 255, 255, 0.1)');
				gradient.addColorStop(0.80, 'rgba(0, 0, 0, 0.1)');
				gradient.addColorStop(1.00, 'rgba(0, 0, 0, 0.3)');
			} else {
				gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.50)');
				gradient.addColorStop(0.10, 'rgba(255, 255, 255, 0.20)');
				gradient.addColorStop(0.40, 'rgba(255, 255, 255, 0.00)');
				gradient.addColorStop(0.50, 'rgba(0, 0, 0, 0.0)');
				gradient.addColorStop(0.90, 'rgba(0, 0, 0, 0.1)');
				gradient.addColorStop(1.00, 'rgba(0, 0, 0, 0.3)');
			}
		}
		return gradient;
	},

	getSmoothGradient : function(element, context, params){
		var gradient = context.createLinearGradient(
			params.x, params.y,
			params.x, params.y+params.h
		);

		gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.08)');
		gradient.addColorStop(1.00, 'rgba(0, 0, 0, 0.25)');

		return gradient;
	},

	getSoftGradient : function(element, context, params){
		var gradient = context.createLinearGradient(
			params.x, params.y, 
			params.x, params.y+params.h
		);

		if (element.selected){
			gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.20)');
			gradient.addColorStop(0.10, 'rgba(255, 255, 255, 0.05)');
			gradient.addColorStop(0.90, 'rgba(255, 255, 255, 0.00)');

		} else {
			if (element.hover){
				gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.30)');
				gradient.addColorStop(0.25, 'rgba(255, 255, 255, 0.18)');
			} else {
				gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.25)');
				gradient.addColorStop(0.10, 'rgba(255, 255, 255, 0.15)');
			}
		}
		return gradient;
	},

	getCleanGradient : function(element, context, params){
		var gradient = context.createLinearGradient(
			params.x, params.y, 
			params.x, params.y+params.h
		);

		gradient.addColorStop(0.00, 'rgba(255, 255, 255, 0.20)');
		gradient.addColorStop(0.10, 'rgba(255, 255, 255, 0.05)');
		gradient.addColorStop(0.90, 'rgba(255, 255, 255, 0.00)');
		return gradient;
	},

	getInnerTextWidth : function(element){
		var w = Native.getTextWidth(
			element._label,
			element._fontSize,
			element._fontFamily
		);
		return element._paddingLeft + Math.round(w) + element._paddingRight;
	}
};

/* -------------------------------------------------------------------------- */

NDMElement.defineNativeProperty = function(descriptor){
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
	Object.createHiddenElement(element, "_"+property, value);

	/* define public accessor */
	Object.defineProperty(element, property, {
		get : function(){
			var r = undefined;
			if (element._locked === false) {
				element.__lock("plugin:"+property);
				r = getter ? getter.call(element) : undefined;
				element.__unlock("plugin:"+property);
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

			if (element.initialized && element._locked === false) {
				/* lock element */
				element.__lock("plugin:"+property);

				/* fire propertyupdate event if needed */
				NDMElement.onPropertyUpdate({
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
				element.__unlock("plugin:"+property);
			}

		},

		enumerable : enumerable,
		configurable : configurable
	});
};

/* -------------------------------------------------------------------------- */

NDMElement.defineDescriptors = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			var descriptor = props[key],
				value = element["_"+key] != undefined ?
						element["_"+key] : undefined;

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

/* -------------------------------------------------------------------------- */

NDMElement.definePublicProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			this.defineNativeProperty({
				element : element,
				property : key,
				value : props[key],
				enumerable : true,
				configurable : true
			});
		}
	}
};

NDMElement.defineReadOnlyProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			Object.createProtectedElement(element, key, props[key]);
		}
	}
};

NDMElement.defineInternalProperties = function(element, props){
	for (var key in props){
		if (props.hasOwnProperty(key)){
			Object.createHiddenElement(element, key, props[key]);
		}
	}
};

/* -------------------------------------------------------------------------- */

