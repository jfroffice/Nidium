/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

Native.elements.export("UIPath", {
	init : function(){
		var o = this.options;
		
		this.path = OptionalString(o.path, "");
		this.units = OptionalNumber(o.units, 1000);
		this.advx = OptionalNumber(o.advx, 1000);
		this.advy = OptionalNumber(o.advy, 1000);
		this.data = Kinetic.Path.parsePathData(this.path);
	},

	draw : function(context){
		var	params = this.getDrawingBounds(),
			scale = this.width / this.units;

		if (this.shadowBlur != 0) {
			context.setShadow(
				this.shadowOffsetX,
				this.shadowOffsetY,
				this.shadowBlur,
				this.shadowColor
			);
		}

		DOMElement.draw.box(this, context, params);

		context.setColor(this.color);
		context.setPath(this.data, this.units, this.advx, this.advy, scale);
		context.setShadow(0, 0, 0);
	}
});