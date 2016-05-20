/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

var dsp;
var node;

Tests.register("SourceNode create", function() {
    dsp = Audio.getContext();
    node = dsp.createNode("source", 0, 2);
    Assert(node instanceof AudioNode);
});

Tests.register("SourceNode with invalid input", function() {
    try {
        dsp.createNode("source", "foobar", "foobar");
    } catch (e) {
        return;
    }

    Assert("Exception was expected");
});

Tests.registerAsync("SourceNode open invalid file", function(next) {
    node.open("invalidfile")
    node.addEventListener("error", function(ev) {
        // XXX : NativeStream triggers two error
        // workaround this issue otherwise callback is executed twice
        node.removeEventListener("error");
        // Remove ready event otherwise it will be fired by the next test
        node.removeEventListener("ready");
        Assert.strictEqual(ev.code, 0, "Invalid error code returned")
        next();
    });

    node.addEventListener("ready", function() {
        Assert(false, "Node fired onready callback oO");
        next();
    });
}, 5000);

Tests.registerAsync("SourceNode open file", function(next) {
    node.open("AV/test.mp3")
    node.addEventListener("error", function(e) {
        node.removeEventListener("error");
        Assert(false, "Failed to open media file");
        next();
    });

    node.addEventListener("ready", function() {
        next();
    });
}, 5000);

Tests.register("SourceNode MetaData", function() {
    Assert(JSON.stringify(node.metadata) == '{"title":"The Sound of Epicness (ID: 358232)","artist":"larrylarrybb","album":"Newgrounds Audio Portal - http://www.newgrounds.com/audio","track":"01/01","streams":[{}]}',
        "File MetaData does not match");
});

Tests.registerAsync("SourceNode play event", function(next) {
    node.addEventListener("play", function() {
        next();
    });

    node.play();
}, 5000);

Tests.registerAsync("SourceNode pause event", function(next) {
    node.addEventListener("pause", function() {
        next();
    });

    node.pause();
}, 5000);

Tests.registerAsync("SourceNode seek", function(next) {
    node.position = 20;

    // Seeking is async
    setTimeout(function() {
        // Truncate the position, because node.position returns a float
        // and seek is not accurate to the milisecond
        Assert.strictEqual((node.position | 0), 20, "Source position is invalid");
        next();
    }, 200);
}, 5000);

Tests.registerAsync("SourceNode stop", function(next) {
    node.addEventListener("stop", function() {
        next();
    });

    node.stop();
}, 5000);

Tests.registerAsync("SourceNode close", function(next) {
    node.close();
    setTimeout(function() {
        next();
    }, 1000);
}, 5000);
