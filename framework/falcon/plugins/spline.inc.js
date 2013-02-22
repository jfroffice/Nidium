/* --------------------------------------------------------------------------- *
 * Native Splines                                          (c) 2013 Stight.com * 
 * --------------------------------------------------------------------------- * 
 * Version:     1.0                                                            *
 * Author:      Vincent Fontaine                                               *
 *                                                                             *
 * Permission is hereby granted, free of charge, to any person obtaining a     *
 * copy of this software and associated documentation files (the "Software"),  *
 * to deal in the Software without restriction, including without limitation   *
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,    *
 * and/or sellcopies of the Software, and to permit persons to whom the        *
 * Software is furnished to do so, subject to the following conditions:        *
 *                                                                             *
 * The above copyright notice and this permission notice shall be included in  *
 * all copies or substantial portions of the Software.                         *
 *                                                                             *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,    *
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE *
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER      *
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING     *
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER         *
 * DEALINGS IN THE SOFTWARE.                                                   *
 * --------------------------------------------------------------------------- * 
 */

Canvas.implement({
	fastSpline : function(points, step, lineWidth=2){
		var len = 0,
			step = 0,
			n = points.length-1,
			k = lineWidth/2,
			bx = [],
			by = [];

		this.beginPath();
		this.moveTo(points[0][0], points[0][1]);
		for (var t=0; t<=1; t+=step){
			var r = [0, 0];

			for (var i=0; i<=n; i++) {
				r[0] += points[i][0] * Math.spline(i, n, t);
				r[1] += points[i][1] * Math.spline(i, n, t);
			}

			bx.push(r[0]);
			by.push(r[1]);

			this.lineTo(r[0], r[1]);
		}
		this.stroke();

		return {
			left : Math.floor(1-k + bx.min()),
			top : Math.floor(1-k + by.min()),
			right : Math.floor(k + bx.max()),
			bottom : Math.floor(k + by.max())
		}
	},

	spline : function(points, mx=0, my=0, lineWidth=2){
		var len = 0,
			step = 0,
			n = points.length-1,
			mouseOnPath = false,
			f = Math.factorial, pw = Math.pow,
			k = lineWidth/2,
			bx = [],
			by = [];

		var B = function(i, n, t){
			return	f(n) / (f(i) * f(n-i)) * pw(t, i) * pw(1-t, n-i);
		}

		for (var i=0; i<n; i++) {
			len += Math.distance(
				points[i][0], points[i][1],
				points[i + 1][0], points[i + 1][1]
			);
		}

		step = 1/len;

		this.beginPath();
		this.moveTo(points[0][0], points[0][1]);
		for (var t=0; t<=1; t+=step){
			var r = [0, 0];

			for (var i=0; i<=n; i++) {
				r[0] += points[i][0] * B(i, n, t);
				r[1] += points[i][1] * B(i, n, t);
			}

			bx.push(r[0]);
			by.push(r[1]);
			mouseOnPath = (Math.distance(r[0], r[1], mx, my) <= lineWidth) ? true : mouseOnPath;

			this.lineTo(r[0], r[1]);
		}
		this.stroke();

		return {
			left : Math.floor(1-k + bx.min()),
			top : Math.floor(1-k + by.min()),
			right : Math.floor(k + bx.max()),
			bottom : Math.floor(k + by.max())
		}
	}
});