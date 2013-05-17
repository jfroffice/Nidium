/* --------------------------------------------------------------------------- *
 * CHARTJS DEMO                                            (c) 2013 Stight.com * 
 * --------------------------------------------------------------------------- * 
 * Original:    http://chartjs.org/                                            *
 * --------------------------------------------------------------------------- * 
 * Released under the MIT license                                              *
 * https://github.com/nnnick/Chart.js/blob/master/LICENSE.md                   *
 * --------------------------------------------------------------------------- * 
 */

/* -------------------------------------------------------------------------- */
include("libs/chart.lib.js");
/* -------------------------------------------------------------------------- */

var	view = document.add("UIElement", {
	width : 650,
	height : 530
}).center();

var doughnutData = [
	{
		value: 30,
		color:"#F7464A"
	},
	{
		value : 50,
		color : "#46BFBD"
	},
	{
		value : 100,
		color : "#FDB45C"
	},
	{
		value : 40,
		color : "#949FB1"
	},
	{
		value : 120,
		color : "#4D5360"
	}
];

var myDoughnut = new Chart(view).Doughnut(doughnutData);
