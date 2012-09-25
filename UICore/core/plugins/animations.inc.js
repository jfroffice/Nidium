/* -------------------------- */
/* Native (@) 2012 Stight.com */
/* -------------------------- */

UIView.implement({
	fadeIn : function(duration, callback){
		var view = this,
			start = view.opacity,
			end = 1 - start,
			slice = 10,
			time = 0;

		//view.visible = true;

		if (view._currentAnimation) {
			view._currentAnimation.remove();
		}
		view._currentAnimation = setTimer(function(){

			view.opacity = FXAnimation.easeOutCubic(0, time, start, end, duration);
			canvas.__mustBeDrawn = true;

			time += slice;

			if (time>duration && view._currentAnimation){
				this.remove();
				if(typeof(callback)=="function") callback.call(view);
			}
		}, slice, true, true);
	},


	fadeOut : function(duration, callback){
		var view = this,
			start = view.opacity,
			end = 0 - start,
			slice = 10,
			time = 0;

		if (view._currentAnimation) {
			view._currentAnimation.remove();
		}
		view._currentAnimation = setTimer(function(){

			view.opacity = FXAnimation.easeOutCubic(0, time, start, end, duration);
			canvas.__mustBeDrawn = true;

			time += slice;

			if (time>duration && view._currentAnimation){
				//view.visible = false;
				this.remove();
				if(typeof(callback)=="function") callback.call(view);
			}
		}, slice, true, true);
	},

	bounceScale : function(scale, duration, callback, fx){
		var view = this,
			start = view.scale,
			end = scale - start,
			slice = 10,
			time = 0;

		if (view._currentAnimation) {
			this.remove();
		}
		view._currentAnimation = setTimer(function(){
			fx = fx || FXAnimation.easeInOutQuad;

			view.scale = fx(0, time, start, end, duration);
			canvas.__mustBeDrawn = true;
			view.opacity = fx(0, time, start, end, duration);

			time += slice;

			if (time>duration && view._currentAnimation){
				this.remove();
				if(typeof(callback)=="function") callback.call(view);
			}
		}, slice, true, true);
	},

	slideX : function(delta, duration, callback, fx){
		var view = this,
			start = view.left,
			end = delta - start,
			slice = 10,
			time = 0;

		if (view._currentAnimation) {
			this.remove();
		}
		view._currentAnimation = setTimer(function(){
			fx = fx || FXAnimation.easeInOutQuad;

			view.left = fx(0, time, start, end, duration);
			canvas.__mustBeDrawn = true;

			time += slice;

			if (time>duration && view._currentAnimation){
				if (this.remove) {
					this.remove();
				}
				if(typeof(callback)=="function") callback.call(view);
			}
		}, slice, true, true);
	},

	scrollY : function(deltaY){
		var self = this,
			startY = this.scroll.top,
			endY = this.scroll.top + deltaY,
			maxY = this.content.height-this.h,
			slice = 10,
			duration = 80;

		if (!this.scroll.initied){
			self.scroll.initied = true;
			self.scroll.time = 0;
			self.scroll.duration = duration;
			self.scroll.startY = startY;
			self.scroll.endY = endY;
			self.scroll.deltaY = deltaY;
			self.scroll.accy = 1.0;
		}

		if (this.scroll.scrolling) {
			self.scroll.timer.remove();
			this.scroll.scrolling = false;
			this.scroll.initied = false;
		}

		if (!this.scroll.scrolling){


			self.scroll.scrolling = true;

			self.scroll.timer = setTimer(function(){
				let stop = false;
		
				self.scroll.top = FXAnimation.easeOutCubic(0, self.scroll.time, startY, deltaY, self.scroll.duration);
				self.scroll.time += slice;

				delete(self.__cache);
				canvas.__mustBeDrawn = true;


				if (deltaY>=0) {
					if (self.scroll.top > maxY) {
						self.scroll.top = maxY;
						stop = true;
					}

					if (self.scroll.top > endY) {
						self.scroll.top = endY;
						stop = true;
					}
				} else {
					if (self.scroll.top < endY) {
						self.scroll.top = endY;
						stop = true;
					}

					if (self.scroll.top < 0) {
						self.scroll.top = 0;
						stop = true;
					}
				}

				if (self.scroll.time>duration){
					stop = true;
				}

				if (stop && this.remove){
					self.scroll.scrolling = false;
					self.scroll.initied = false;
					this.remove();
				}

			}, slice, true, true);

		}

	},
	
	animate : function(property, from, delta, duration, callback, fx, rtCallback){
		var view = this,
			start = from,
			end = delta - start,
			slice = 10,
			time = 0;

		if (view._currentAnimation) {
			this.remove();
		}
		view._currentAnimation = setTimer(function(){
			fx = fx || FXAnimation.easeInOutQuad;

			view[property] = fx(0, time, start, end, duration);
			canvas.__mustBeDrawn = true;
			if(typeof(rtCallback)=="function") rtCallback.call(view, view[property]);

			time += slice;

			if (this.remove && time>duration && view._currentAnimation){
				//view.visible = false;
				this.remove();
				view[property] = from+end;
				canvas.__mustBeDrawn = true;
				if(typeof(callback)=="function") callback.call(view);
			}
		}, slice, true, true);
	}

});



/*
 * Open source under the BSD License. 
 * 
 * Copyright (c) 2008 George McGinley Smith
 * All rights reserved.
 * 
 * Redistribution and use in source and binary forms, with or without modification, 
 * are permitted provided that the following conditions are met:
 * 
 * Redistributions of source code must retain the above copyright notice, this list of 
 * conditions and the following disclaimer.
 * Redistributions in binary form must reproduce the above copyright notice, this list 
 * of conditions and the following disclaimer in the documentation and/or other materials 
 * provided with the distribution.
 * 
 * Neither the name of the author nor the names of contributors may be used to endorse 
 * or promote products derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY 
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 * COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED 
 * AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED 
 * OF THE POSSIBILITY OF SUCH DAMAGE. 
 *
*/

// t: current time, b: beginning value, c: change In value, d: duration

var FXAnimation = {
	def: 'easeOutQuad',
	
	swing: function (x, t, b, c, d) {
		return FXAnimation[FXAnimation.def](x, t, b, c, d);
	},
	
	easeInQuad: function (x, t, b, c, d) {
		return c*(t/=d)*t + b;
	},
	
	easeOutQuad: function (x, t, b, c, d) {
		return -c *(t/=d)*(t-2) + b;
	},
	
	easeInOutQuad: function (x, t, b, c, d) {
		if ((t/=d/2) < 1) return c/2*t*t + b;
		return -c/2 * ((--t)*(t-2) - 1) + b;
	},
	
	easeInCubic: function (x, t, b, c, d) {
		return c*(t/=d)*t*t + b;
	},
	
	easeOutCubic: function (x, t, b, c, d) {
		return c*((t=t/d-1)*t*t + 1) + b;
	},
	
	easeInOutCubic: function (x, t, b, c, d) {
		if ((t/=d/2) < 1) return c/2*t*t*t + b;
		return c/2*((t-=2)*t*t + 2) + b;
	},
	
	easeInQuart: function (x, t, b, c, d) {
		return c*(t/=d)*t*t*t + b;
	},
	
	easeOutQuart: function (x, t, b, c, d) {
		return -c * ((t=t/d-1)*t*t*t - 1) + b;
	},
	
	easeInOutQuart: function (x, t, b, c, d) {
		if ((t/=d/2) < 1) return c/2*t*t*t*t + b;
		return -c/2 * ((t-=2)*t*t*t - 2) + b;
	},
	
	easeInQuint: function (x, t, b, c, d) {
		return c*(t/=d)*t*t*t*t + b;
	},
	
	easeOutQuint: function (x, t, b, c, d) {
		return c*((t=t/d-1)*t*t*t*t + 1) + b;
	},
	
	easeInOutQuint: function (x, t, b, c, d) {
		if ((t/=d/2) < 1) return c/2*t*t*t*t*t + b;
		return c/2*((t-=2)*t*t*t*t + 2) + b;
	},
	
	easeInSine: function (x, t, b, c, d) {
		return -c * Math.cos(t/d * (Math.PI/2)) + c + b;
	},
	
	easeOutSine: function (x, t, b, c, d) {
		return c * Math.sin(t/d * (Math.PI/2)) + b;
	},
	
	easeInOutSine: function (x, t, b, c, d) {
		return -c/2 * (Math.cos(Math.PI*t/d) - 1) + b;
	},
	
	easeInExpo: function (x, t, b, c, d) {
		return (t==0) ? b : c * Math.pow(2, 10 * (t/d - 1)) + b;
	},
	
	easeOutExpo: function (x, t, b, c, d) {
		return (t==d) ? b+c : c * (-Math.pow(2, -10 * t/d) + 1) + b;
	},
	
	easeInOutExpo: function (x, t, b, c, d) {
		if (t==0) return b;
		if (t==d) return b+c;
		if ((t/=d/2) < 1) return c/2 * Math.pow(2, 10 * (t - 1)) + b;
		return c/2 * (-Math.pow(2, -10 * --t) + 2) + b;
	},
	
	easeInCirc: function (x, t, b, c, d) {
		return -c * (Math.sqrt(1 - (t/=d)*t) - 1) + b;
	},
	
	easeOutCirc: function (x, t, b, c, d) {
		return c * Math.sqrt(1 - (t=t/d-1)*t) + b;
	},
	
	easeInOutCirc: function (x, t, b, c, d) {
		if ((t/=d/2) < 1) return -c/2 * (Math.sqrt(1 - t*t) - 1) + b;
		return c/2 * (Math.sqrt(1 - (t-=2)*t) + 1) + b;
	},
	
	easeInElastic: function (x, t, b, c, d) {
		var s=1.70158;var p=0;var a=c;
		if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
		if (a < Math.abs(c)) { a=c; var s=p/4; }
		else var s = p/(2*Math.PI) * Math.asin (c/a);
		return -(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
	},
	
	easeOutElastic: function (x, t, b, c, d) {
		var s=1.70158;var p=0;var a=c;
		if (t==0) return b;  if ((t/=d)==1) return b+c;  if (!p) p=d*.3;
		if (a < Math.abs(c)) { a=c; var s=p/4; }
		else var s = p/(2*Math.PI) * Math.asin (c/a);
		return a*Math.pow(2,-10*t) * Math.sin( (t*d-s)*(2*Math.PI)/p ) + c + b;
	},

	easeInOutElastic: function (x, t, b, c, d) {
		var s=1.70158;var p=0;var a=c;
		if (t==0) return b;  if ((t/=d/2)==2) return b+c;  if (!p) p=d*(.3*1.5);
		if (a < Math.abs(c)) { a=c; var s=p/4; }
		else var s = p/(2*Math.PI) * Math.asin (c/a);
		if (t < 1) return -.5*(a*Math.pow(2,10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )) + b;
		return a*Math.pow(2,-10*(t-=1)) * Math.sin( (t*d-s)*(2*Math.PI)/p )*.5 + c + b;
	},
	
	easeInBack: function (x, t, b, c, d, s) {
		if (s == undefined) s = 1.70158;
		return c*(t/=d)*t*((s+1)*t - s) + b;
	},
	
	easeOutBack: function (x, t, b, c, d, s) {
		if (s == undefined) s = 1.70158;
		return c*((t=t/d-1)*t*((s+1)*t + s) + 1) + b;
	},
	
	easeInOutBack: function (x, t, b, c, d, s) {
		if (s == undefined) s = 1.70158; 
		if ((t/=d/2) < 1) return c/2*(t*t*(((s*=(1.525))+1)*t - s)) + b;
		return c/2*((t-=2)*t*(((s*=(1.525))+1)*t + s) + 2) + b;
	},
	
	easeInBounce: function (x, t, b, c, d) {
		return c - FXAnimation.easeOutBounce(x, d-t, 0, c, d) + b;
	},
	
	easeOutBounce: function (x, t, b, c, d) {
		if ((t/=d) < (1/2.75)) {
			return c*(7.5625*t*t) + b;
		} else if (t < (2/2.75)) {
			return c*(7.5625*(t-=(1.5/2.75))*t + .75) + b;
		} else if (t < (2.5/2.75)) {
			return c*(7.5625*(t-=(2.25/2.75))*t + .9375) + b;
		} else {
			return c*(7.5625*(t-=(2.625/2.75))*t + .984375) + b;
		}
	},
	
	easeInOutBounce: function (x, t, b, c, d) {
		if (t < d/2) return FXAnimation.easeInBounce (x, t*2, 0, c, d) * .5 + b;
		return FXAnimation.easeOutBounce(x, t*2-d, 0, c, d) * .5 + c*.5 + b;
	}
};