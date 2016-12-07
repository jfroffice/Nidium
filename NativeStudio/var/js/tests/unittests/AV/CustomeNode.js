/*
   Copyright 2016 Nidium Inc. All rights reserved.
   Use of this source code is governed by a MIT license
   that can be found in the LICENSE file.
*/

var dsp, node;
Tests.register("CustomNode.create", function() {
    dsp = Audio.getContext();
    node = dsp.createNode("custom", 2, 2);
    Assert(node instanceof AudioNode);
});

Tests.registerAsync("CustomNode.set(key, value)", function(next) {
    node.assignSetter(function(key, value) {
        this.send({"key": key, "value": value});
    })

    node.set("foo", "bar");
    node.addEventListener("message", function(ev) {
        node.assignSetter(null);
        node.removeEventListener("message");
        Assert.equal(ev.data.key, "foo", "Key isn't \"foo\"")
        Assert.equal(ev.data.value, "bar", "Value isn't \"bar\"");
        next()
    });
}, 5000);

Tests.registerAsync("CustomNode.set(object)", function(next) {
    node.assignSetter(function(key, value) {
        this.send({"key": key, "value": value});
    });

    node.set({"ILove": "Rock'n'Roll"});

    node.addEventListener("message", function(msg) {
        node.setter = null;
        Assert(msg.data.key == "ILove" && msg.data.value == "Rock'n'Roll");
        next();
    });
}, 5000);

Tests.register("CustomNode.set(invalid data)", function() {
    var thrown = false;
    try {
        node.set(null);
    } catch (e) {
        thrown = true;
    }

    Assert(thrown, ".set(null) didn't throw an error");


    thrown = false;
    try {
        node.set(null, "foo");
    } catch (e) {
        thrown = true;
    }

    Assert(thrown, ".set(null, \"foo\") didn't throw an error");
});
