/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

/* -------------------------------------------------------------------------- */
/* NSS PROPERTIES                                                             */
/* -------------------------------------------------------------------------- */

document.nss.add({
	"UILabel" : {
		canReceiveFocus : false,
		label : "Label",
		fontSize : 11,
		fontFamily : "arial",
		textAlign : "left",

		textShadowOffsetX : 1,
		textShadowOffsetY : 1,
		textShadowBlur : 1,
		textShadowColor : 'rgba(0, 0, 0, 0.06)',

		autowidth : true,
		height : 22,
		radius : 0,
		background : "",
		color : "#222222"
	}
});

/* -------------------------------------------------------------------------- */
/* ELEMENT DEFINITION                                                         */
/* -------------------------------------------------------------------------- */

Native.elements.export("UILabel", {
	init : function(){
		var o = this.options;

		/* Element's Specific Dynamic Properties */
		NDMElement.defineDynamicProperties(this, {
			autowidth : OptionalBoolean(o.autowidth, true)
		});

		this.addEventListener("dragstart", function(e){
			e.forcePropagation();
		}, true);

		this.addEventListener("drag", function(e){
			e.forcePropagation();
		}, true);

		this.addEventListener("dragend", function(e){
			e.forcePropagation();
		}, true);
	},

	update : function(e){
		if (e.property.in(
			"width", "height",
			"label", "fontSize", "fontFamily",
			"paddingLeft", "paddingRight"
		)) {
			this.resize();
		}
	},

	resize : function(){
		if (this.autowidth) {
			this.width = NDMElement.draw.getInnerTextWidth(this);
		}
	},

	draw : function(context){
		var	params = this.getDrawingBounds();
		NDMElement.draw.box(this, context, params);
		NDMElement.draw.label(this, context, params);
	}
});
