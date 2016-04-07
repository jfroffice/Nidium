/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

Tests.register("Global.btoa", function() {
	Assert.equal(btoa("Hello Nidium"), "SGVsbG8gTmlkaXVt");
	Assert.equal(btoa("hello nidium"), "aGVsbG8gbmlkaXVt");
});

