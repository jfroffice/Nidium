/* -------------------------- */
/* Native (@) 2012 Stight.com */
/* -------------------------- */

var main = new Application(),

	// Small Green Slider
	slider1 = main.add("UISliderController", {
		left : 350,
		top : 150,
		background : '#161712',
		color : 'rgba(210, 255, 40, 1)',
		progressBarColor : 'rgba(210, 255, 40, 1)',
		disabled : false,
		radius : 2,
		min : -100,
		max : 100,
		value : 0
	}),

	// Small Rose Slider
	slider2 = main.add("UISliderController", {
		left : 350,
		top : 180,
		background : '#161712',
		color : 'rgba(255, 40, 210, 1)',
		progressBarColor : 'rgba(255, 40, 210, 1)',
		disabled : false,
		radius : 2,
		min : -100,
		max : 100,
		value : 0
	}),

	// Big Blue Slider
	slider3 = main.add("UISliderController", {
		left : 350,
		top : 240,
		width : 250,
		height : 20,

		displayLabel : true,
		labelBackground : 'rgba(255, 255, 255, 0.7)',
		labelColor : 'rgba(0, 0, 0, 0.5)',
		labelOffset : -18,
		labelWidth : 56,
		labelPrefix : 'f=',
		labelSuffix : ' %',
		fontSize : 10,
		lineHeight : 18,

		splitColor : 'rgba(0, 0, 0, 0.5)',
		boxColor : 'rgba(255, 255, 255, 0.02)',
		
		disabled : false,
		radius : 2,
		min : -1,
		max : 1,
		value : 0
	}),

	master = main.add("UISliderController", {
		left : 800,
		top : 210,
		width : 12,
		height : 300,
		vertical : true,
		background : '#161712',
		color : 'rgba(255, 40, 210, 1)',
		progressBarColor : 'rgba(255, 40, 210, 1)',
		splitColor : 'rgba(50, 40, 10, 0.7)',
		disabled : false,
		radius : 3,
		min : 0,
		max : 300,
		value : 0
	});


master.addEventListener("complete", function(value){
	echo(value);
}, false);



// Big Blue Slider controls the small sliders
slider3.addEventListener("update", function(value){
	var g1 = slider1.value,
		g2 = slider2.value;

	slider1.setValue(g1+value*20);
	slider2.setValue(g2+value*20);
	frequency = (value+1)/5;
}, false);



/*
 * -- Sine Sliders -----------------
 */

/*

var sliders = [],
	nb_sliders = 16,
	frequency = 4.8,
	sliderTop = 90,
	sliderLeft = 90,
	sliderWidth = 140,
	gdSpectrum = Native.canvas.ctx.createLinearGradient(0, 0, sliderWidth, 0);

gdSpectrum.addColorStop(0.00,'#002200');
gdSpectrum.addColorStop(0.25,'#00ff00');
gdSpectrum.addColorStop(0.50,'#ffff00');
gdSpectrum.addColorStop(1.00,'#ff0000');

for (var s=0; s<nb_sliders; s++){
	sliders[s] = main.add("UISliderController", {
		left : sliderLeft,
		top : 100 + s*36,
		width : sliderWidth,
		height : 20,
		background : '#161712',
		color : '#111111',
		boxColor : 'rgba(0, 0, 0, 1)',
		progressBarColor : gdSpectrum,
		splitColor : 'rgba(60, 60, 60, 0.40)',
		ease : Math.physics.cubicIn,
		radius : 3,
		min : -20,
		max : 20,
		value : 0
	});
}

setTimeout(function(){
	slider1.setValue(-50, 400);

	var s = 0,
		time = 0;
		t = Native.timer(function(){
			sliders[s].setValue(20*Math.sin(3*frequency*time++), 200);

			s++;
			if (s >= nb_sliders) {
				s = 0;
			}
		}, 60, true, true);



}, 1500);

*/