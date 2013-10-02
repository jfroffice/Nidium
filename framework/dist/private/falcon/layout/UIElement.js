/* ------------------------+------------- */
/* Native Framework 2.0    | Falcon Build */
/* ------------------------+------------- */
/* (c) 2013 nidium.com - Vincent Fontaine */
/* -------------------------------------- */

Native.elements.export("UIElement", {
	init : function(){
		/* dummy */
		NDMElement.listeners.addHovers(this);
	},

	draw : function(context){
		var	params = this.getDrawingBounds();

		NDMElement.draw.enableShadow(this);
		NDMElement.draw.box(this, context, params);
		NDMElement.draw.disableShadow(this);
	}
});