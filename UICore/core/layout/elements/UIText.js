/* -------------------------- */
/* Native (@) 2012 Stight.com */
/* -------------------------- */

UIElement.extend("UIText", {
	init : function(){
		var self = this;

		this.flags._canReceiveFocus = true;

		if (!this.color){
			this.color = "#000000";
		}

		this.padding = {
			left : 0,
			right : 0,
			top : 0,
			bottom : 0
		};

		this.setText = function(text){
			this._textMatrix = getTextMatrixLines(text, this.lineHeight, this.w, this.textAlign, this.fontSize);
			this.content.height = this.lineHeight * this._textMatrix.length;

			this.mouseSelectionArea = null;

			this.selection = {
				text : '',
				position : 0,
				size : 0
			};

			this.caret = {
				x1 : 0,
				y1 : 0,
				x2 : 0,
				y2 : 0
			};

		};

		this.addEventListener("mouseover", function(e){
			this.verticalScrollBar.fadeIn(150, function(){
				/* dummy */
			});
		}, false);

		this.addEventListener("mouseout", function(e){
			this.verticalScrollBar.fadeOut(400, function(){
				/* dummy */
			});
		}, false);

		this.addEventListener("dragstart", function(e){
			this.__startTextSelectionProcessing = true;
			this.__startx = e.x;
			this.__starty = e.y + this.scroll.top;
		}, false);

		layout.rootElement.addEventListener("dragover", function(e){
			if (self.__startTextSelectionProcessing) {
				self.mouseSelectionArea = {
					x1 : self.__startx,
					y1 : self.__starty,
					x2 : e.x,
					y2 : e.y + self.scroll.top
				};

				let area = self.mouseSelectionArea;

				if (area.y2 <= self.__starty) {
					let x1 = area.x2,
						y1 = Math.min(area.y1, area.y2),
						x2 = area.x1,
						y2 = Math.max(area.y1, area.y2);

					area = {
						x1 : x1,
						y1 : y1,
						x2 : x2,
						y2 : y2
					};
				}

				var x = self._x + self.padding.left,
					y = self._y + self.padding.top,

					r = {
						x1 : (area.x1 - x),
						y1 : Math.floor((area.y1 - y) / self.lineHeight) * self.lineHeight,
						x2 : (area.x2 - x),
						y2 : Math.ceil((area.y2 - y) / self.lineHeight) * self.lineHeight
					};

				r.x1 = isNaN(r.x1) ? 0 : r.x1;
				r.y1 = isNaN(r.y1) ? 0 : r.y1;
				r.x2 = isNaN(r.x2) ? 0 : r.x2;
				r.y2 = isNaN(r.y2) ? 0 : r.y2;

				self.setCaretFromRelativeMouseSelection(r);
				self.selection = self.getTextSelectionFromCaret(self.caret);

			}
		}, false);

		this.addEventListener("dragend", function(e){
			this.__startTextSelectionProcessing = false;
		}, false);

		this.addEventListener("mousedown", function(e){
			if (this.mouseSelectionArea) {
				this.mouseSelectionArea = null;
			}
			if (this.scroll.scrolling){
				Timers.remove(this.scroll.timer);
				this.scroll.scrolling = false;
				this.scroll.initied = false;
			}
		}, false);

		this.addEventListener("mousewheel", function(e){
			if (this.h / this.scrollBarHeight < 1) {
				canvas.__mustBeDrawn = true;
				this.scrollY(1 + (-e.yrel-1) * 18);
			}
		}, false);

		this.addEventListener("focus", function(e){
		}, false);


		/* -------------------------------------------------------------------------------- */

		this.setCaretFromRelativeMouseSelection = function(r){
			var m = this._textMatrix;
				x1 = r.x1,
				x2 = r.x2,
				c = {
					y1 : r.y1/this.lineHeight,
					y2 : r.y2/this.lineHeight - 1
				};

			var getCharPosition = function(line, x, flag){
				let i = 0, l = line.letters;
				while (i<l.length && x > l[i].position + (flag ? l[i].width : 0) ){	i++; }
				return i;
			};

			c.y1 = c.y1.bound(0, m.length-1);
			c.y2 = c.y2.bound(0, m.length-1);
			c.x1 = getCharPosition(m[c.y1], x1) -1; // letters of line y1
			c.x2 = getCharPosition(m[c.y2], x2, true); // letters of line y2

		 	if (c.x2 < 0){
		 		c.x2 = m[--c.y2].letters.length - 1;
		 	}

		 	if (c.x2 > m[c.y2].letters.length - 1){
		 		c.x2 = m[c.y2].letters.length - 1;
		 	}

		 	if (c.x1 < 0){
		 		c.x1 = 0;
		 	}

			this.caret = c;
		};

		this.select = function(state){
			let m = this._textMatrix,
				nb_lines,
				y = 0,
				x = 0,
				chars = [],
				nb_chars = m[y].letters.length - 1,
				nb_lines = m.length - 1;
			
			state = (typeof(state) == "undefined") ? true : state ? true : false;

		 	while ( !(y >= nb_lines && x >= nb_chars) ){
		 		chars = m[y].letters;
		 		nb_chars = chars.length - 1;
		 		chars[x++].selected = state;

		 		if (x > nb_chars) {
			 		x = 0;
					y++;
		 		}
		 	}

		};

		this.getTextSelectionFromCaret = function(c){
			var m = this._textMatrix;
			
			if (c.y2 <= c.y1) {
				let x1 = Math.min(c.x1, c.x2),
					y1 = Math.min(c.y1, c.y2),
					x2 = Math.max(c.x1, c.x2),
					y2 = Math.max(c.y1, c.y2);

				c = {
					x1 : x1,
					y1 : y1,
					x2 : x2,
					y2 : y2
				};
			}

		 	var line = c.y1,
		 		char = c.x1,
				size = 0,
				selection = [],
		 		doit = true,
		 		lastLetter = false;

		 	var selectLetter = function(line, char){
		 		let letter = m[line].letters[char],
		 			nush = m[line].letters[char+1] ? m[line].letters[char+1].position - letter.position - letter.width : 0;

		 		letter.selected = true;
		 		letter.fullWidth = letter.width + nush + 0.25
		 		selection.push(letter.char);
		 		size++;
		 	};

		 	this.selectFor(false);

		 	while ( !(line >= c.y2 && char >= c.x2) ){
		 		let len = m[line].letters.length - 1;
				selectLetter(line, char++);
		 		if (char > len) {
			 		char = 0;
					line++;
		 		}
		 	}

			selectLetter(line, char);

		 	return {
		 		text : selection.join(''),
		 		offset : 0,
		 		size : size
		 	};

		};


		this.verticalScrollBar = this.add("UIVerticalScrollBar");
		this.verticalScrollBarHandle = this.verticalScrollBar.add("UIVerticalScrollBarHandle");

		this.setText(this.text);

	},

	draw : function(){
		var params = {
				x : this._x,
				y : this._y,
				w : this.w,
				h : this.h
			},

			x = params.x + this.padding.left,
			y = params.y + this.padding.top,
			w = params.w - this.padding.right - this.padding.left,
			h = params.h - this.padding.top - this.padding.bottom,

			vOffset = (this.lineHeight/2)+5;


		//if (this.__cache) {
			//canvas.putImageData(this.__cache, params.x, params.y);
		//} else {

			canvas.save();
				if (this.background){
					canvas.fillStyle = this.background;
				}
				canvas.roundbox(params.x, params.y, params.w, params.h, this.radius, this.background, false); // main view
				canvas.clip();
		
				if (this.mouseSelectionArea) {
					//this.selection = drawCaretSelection(this._textMatrix, this.caret, x, y - this.scroll.top, this.lineHeight);
				}

				printTextMatrix(this._textMatrix, x, y - this.scroll.top, vOffset, w, h, params.y, this.lineHeight, this.fontSize, this.fontType, this.color);

			canvas.restore();
			//this.__cache = canvas.getImageData(params.x, params.y, params.w, params.h);
		//}

	}
});

function drawCaretSelection(textMatix, caret, x, y, lineHeight){
	var m = textMatix,
		cx = x,
		cy = y,
		cw = 2,
		c = caret;

	canvas.fillStyle = "rgba(180, 180, 255, 0.60)";

	if (c.y2 <= c.y1) {
		let x1 = Math.min(c.x1, c.x2),
			y1 = Math.min(c.y1, c.y2),
			x2 = Math.max(c.x1, c.x2),
			y2 = Math.max(c.y1, c.y2);

		c = {
			x1 : x1,
			y1 : y1,
			x2 : x2,
			y2 : y2
		};
	}

 	var line = c.y1,
 		char = c.x1,
		size = 0,
		selection = [],
 		doit = true,
 		lastLetter = false;

 	var selectLetter = function(line, char){
 		let letter = m[line].letters[char],
 			nush = m[line].letters[char+1] ? m[line].letters[char+1].position - letter.position - letter.width : 0;

 		cx = x + letter.position;
 		cy = y + line * lineHeight + 1;
 		cw = letter.width + nush + 0.25;
 		letter.selected = true;
		//canvas.fillRect(cx, cy, cw, lineHeight);
 		selection.push(letter.char);
 		size++;
 	};

 	while ( !(line >= c.y2 && char >= c.x2) ){
 		let len = m[line].letters.length - 1;
		selectLetter(line, char++);
 		if (char > len) {
	 		char = 0;
			line++;
 		}
 	}

	selectLetter(line, char);

 	return {
 		size : size,
 		text : selection.join('')
 	};

}


canvas.implement({
	highlightLetters : function(letters, x, y, lineHeight){
		let c, nush, cx, cy, cw;
		for (var i=0; i<letters.length; i++){
			c = letters[i];
	 		nush = letters[i+1] ? letters[i+1].position - c.position - c.width : 0;
		 	cx = x + c.position;
 			cy = y;
 			cw = c.width + nush + 0.25;
 			if (c.selected){
				this.fillRect(cx, cy, cw, lineHeight);
			}
		}
	},

	drawLetters : function(letters, x, y){
		let c;
		for (var i=0; i<letters.length; i++){
			c = letters[i];
			this.fillText(c.char, x + c.position, y);
		}
	}
});

function getLineLetters(wordsArray, textAlign, fitWidth, fontSize){
	var context = new Canvas(640, 480),
		widthOf = context.measureText,
		textLine = wordsArray.join(' '),
		
		nb_words = wordsArray.length,
		nb_spaces = nb_words-1,
		nb_letters = textLine.length - nb_spaces,

		spacewidth = 0,
		spacing = (textAlign == "justify") ? fontSize/3 : fontSize/10,

		linegap = 0,
		gap = 0,
		offgap = 0,

		offset = 0,
		__cache = [],

		position = 0,
		letters = [];

	// cache width of letters
	var cachedLetterWidth = function(char){
		return __cache[char] ? __cache[char] : __cache[char] = widthOf(char);
	}

	context.fontSize = fontSize;
	spacewidth = widthOf(" ");
	linegap = fitWidth - widthOf(textLine);

	switch(textAlign) {
		case "justify" :
			gap = (linegap/(textLine.length-1));
			break;
		case "right" :
			offset = linegap;
			break;
		case "center" :
			offset = linegap/2;
			break;
		default :
			break;
	}

	for (var i=0; i<textLine.length; i++){
		let char = textLine[i],
			letterWidth = cachedLetterWidth(char);

		if (textAlign=="justify"){
			if (char==" "){
				letterWidth += spacing;
			} else {
				letterWidth -= spacing*nb_spaces/nb_letters;
			}
		}
		/*
		if ( char.charCodeAt(0) == 9) {
			letterWidth = 6 * spacewidth;
		};
		*/

		letters[i] = {
			char : char,
			position : offset + position + offgap,
			width : letterWidth,
			linegap : linegap,
			selected : false
		};
		position += letterWidth;
		offgap += gap;
	}

	// last letter Position Approximation Corrector
	var last = letters[textLine.length-1],
		delta = fitWidth - (last.position + last.width);

	if ((0.05 + last.position + last.width) > fitWidth) {
		last.position = Math.floor(last.position - delta - 0.5);
	}

	return letters;
}

function getTextMatrixLines(text, lineHeight, fitWidth, textAlign, fontSize){
	var	paragraphe = text.split(/\r\n|\r|\n/),
		wordsArray = [],
		currentLine = 0,
		context = new Canvas(1, 1);

	var matrix = [];

	context.fontSize = fontSize;

	for (var i = 0; i < paragraphe.length; i++) {
		var words = paragraphe[i].split(' '),
			idx = 1;

		while (words.length > 0 && idx <= words.length) {
			var str = words.slice(0, idx).join(' '),
				w = context.measureText(str);

			if (w > fitWidth) {
				idx = (idx == 1) ? 2 : idx;

				wordsArray = words.slice(0, idx - 1);

				matrix[currentLine++] = {
					text : wordsArray.join(' '),
					align : textAlign,
					words : wordsArray,
					letters : getLineLetters(wordsArray, textAlign, fitWidth, fontSize)
				};
				
				words = words.splice(idx - 1);
				idx = 1;

			} else {
				idx++;
			}

		}

		// last line
		if (idx > 0) {

			let align = (textAlign=="justify") ? "left" : textAlign;
			matrix[currentLine] = {
				text : words.join(' '),
				align : align,
				words : words,
				letters : getLineLetters(words, align, fitWidth, fontSize)
			};

		}

		currentLine++;
	}

	//UIView.content.height = currentLine*lineHeight;

	return matrix;

};

function printTextMatrix(textMatrix, x, y, vOffset, viewportWidth, viewportHeight, viewportTop, lineHeight, fontSize, fontType, color){
	canvas.fontSize = fontSize;
	canvas.fontType = fontType;

	for (var line=0; line<textMatrix.length; line++){
		var tx = x,
			ty = y + vOffset + lineHeight * line;

		// only draw visible lines
		if ( ty < (viewportTop + viewportHeight + lineHeight) && ty >= viewportTop) {
			canvas.fillStyle = "rgba(180, 180, 255, 0.60)";
			canvas.highlightLetters(textMatrix[line].letters, tx, ty - vOffset, lineHeight);
		
			canvas.fillStyle = color;
			canvas.drawLetters(textMatrix[line].letters, tx, ty);
		}
	}
}


