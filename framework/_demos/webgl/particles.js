/*
var vertexShader = "attribute float size;"+
"attribute vec4 ca;"+
"varying vec4 vColor;"+
"void main() {"+
"    vColor = ca;"+
"    vec4 mvPosition = modelViewMatrix * vec4( position, 1.0 );"+
"    gl_PointSize = size * ( 150.0 / length( mvPosition.xyz ) );"+
"    gl_Position = projectionMatrix * mvPosition;"+
"}";


var fragmentShader = "uniform vec3 color;"+
"uniform sampler2D texture;"+
"varying vec4 vColor;"+
"void main() {"+
"    vec4 outColor = texture2D( texture, gl_PointCoord );"+
"    if ( outColor.a < 0.5 ) discard;"+
"    gl_FragColor = outColor * vec4( color * vColor.xyz, 1.0 );"+
"    float depth = gl_FragCoord.z / gl_FragCoord.w;"+
"    const vec3 fogColor = vec3( 0.0 );"+
"    float fogFactor = smoothstep( 200.0, 600.0, depth );"+
"    gl_FragColor = mix( gl_FragColor, vec4( fogColor, gl_FragColor.w ), fogFactor );"+
"}";


var renderer, scene, camera, stats;

var object, uniforms, attributes;

var vc1;

var WIDTH = window.innerWidth;
var HEIGHT = window.innerHeight;

init();
animate();
function init() {

    camera = new THREE.PerspectiveCamera( 40, WIDTH / HEIGHT, 1, 1000 );
    camera.position.z = 500;

    scene = new THREE.Scene();

    attributes = {

        size: { type: 'f', value: [] },
        ca:   { type: 'c', value: [] }

    };

    uniforms = {

        amplitude: { type: "f", value: 1.0 },
        color:     { type: "c", value: new THREE.Color( 0xffffff ) },
        texture:   { type: "t", value: THREE.ImageUtils.loadTexture( "http://f.z.nf/ball.png" ) },

    };

    uniforms.texture.value.wrapS = uniforms.texture.value.wrapT = THREE.RepeatWrapping;

    var shaderMaterial = new THREE.ShaderMaterial( {

        uniforms:       uniforms,
        attributes:     attributes,
        vertexShader:   vertexShader,
        fragmentShader: fragmentShader

    });


    var radius = 100, inner = 0.6 * radius;
    var geometry = new THREE.Geometry();

    for ( var i = 0; i < 100000; i ++ ) {

        var vertex = new THREE.Vector3();
        vertex.x = Math.random() * 2 - 1;
        vertex.y = Math.random() * 2 - 1;
        vertex.z = Math.random() * 2 - 1;
        vertex.multiplyScalar( radius );

        if ( ( vertex.x > inner || vertex.x < -inner ) ||
             ( vertex.y > inner || vertex.y < -inner ) ||
             ( vertex.z > inner || vertex.z < -inner )  )

        geometry.vertices.push( vertex );

    }

    vc1 = geometry.vertices.length;

    var m, dummyMaterial = new THREE.MeshFaceMaterial();

    radius = 200;
    var geometry2 = new THREE.CubeGeometry( radius, 0.1 * radius, 0.1 * radius, 50, 5, 5 );

    function addGeo( geo, x, y, z, ry ) {

        m = new THREE.Mesh( geo, dummyMaterial );
        m.position.set( x, y, z );
        m.rotation.y = ry;

        THREE.GeometryUtils.merge( geometry, m );

    }

    // side 1

    addGeo( geometry2, 0,  110,  110, 0 );
    addGeo( geometry2, 0,  110, -110, 0 );
    addGeo( geometry2, 0, -110,  110, 0 );
    addGeo( geometry2, 0, -110, -110, 0 );

    // side 2

    addGeo( geometry2,  110,  110, 0, Math.PI/2 );
    addGeo( geometry2,  110, -110, 0, Math.PI/2 );
    addGeo( geometry2, -110,  110, 0, Math.PI/2 );
    addGeo( geometry2, -110, -110, 0, Math.PI/2 );

    // corner edges

    var geometry3 = new THREE.CubeGeometry( 0.1 * radius, radius * 1.2, 0.1 * radius, 5, 60, 5 );

    addGeo( geometry3,  110, 0,  110, 0 );
    addGeo( geometry3,  110, 0, -110, 0 );
    addGeo( geometry3, -110, 0,  110, 0 );
    addGeo( geometry3, -110, 0, -110, 0 );

    // particle system

    object = new THREE.ParticleSystem( geometry, shaderMaterial );
    object.dynamic = true;

    // custom attributes

    var vertices = object.geometry.vertices;

    var values_size = attributes.size.value;
    var values_color = attributes.ca.value;

    for( var v = 0; v < vertices.length; v ++ ) {

        values_size[ v ] = 10;
        values_color[ v ] = new THREE.Color( 0xffffff );

        if ( v < vc1 ) {

            values_color[ v ].setHSV( 0.5 + 0.2 * ( v / vc1 ), 0.99, 1.0 );

        } else {

            values_size[ v ] = 55;
            values_color[ v ].setHSV( 0.1, 0.99, 1.0 );

        }

    }

    //console.log( vertices.length );

    scene.add( object );

    renderer = new THREE.WebGLRenderer( { clearColor: 0xFFFFFF, clearAlpha: 1 } );
    renderer.setSize( WIDTH, HEIGHT );


}

function onWindowResize() {

    camera.aspect = window.innerWidth / window.innerHeight;
    camera.updateProjectionMatrix();

    renderer.setSize( window.innerWidth, window.innerHeight );

}

function animate() {

    canvas.requestAnimationFrame( animate );

    render();

}

function render() {

    var time = Date.now() * 0.01;

    object.rotation.y = object.rotation.z = 0.02 * time;

    for( var i = 0; i < attributes.size.value.length; i ++ ) {

        if ( i < vc1 )
            attributes.size.value[ i ] = Math.max(0, 26 + 32 * Math.sin( 0.1 * i + 0.6 * time ));


    }

    attributes.size.needsUpdate = true;
    renderer.render( scene, camera );
}

*/

var VERTEXSHADER = "attribute float size;"+
"attribute vec4 ca;"+
"varying vec4 vColor;"+
"void main() {"+
"    vColor = ca;"+
"    vec4 mvPosition = modelViewMatrix * vec4( position, 1.0 );"+
"    gl_PointSize = size * ( 150.0 / length( mvPosition.xyz ) );"+
"    gl_Position = projectionMatrix * mvPosition;"+
"}";


var FRAGMENTSHADER = "uniform vec3 color;"+
"uniform sampler2D texture;"+
"varying vec4 vColor;"+
"void main() {"+
"    vec4 outColor = texture2D( texture, gl_PointCoord );"+
"    if ( outColor.a < 0.5 ) discard;"+
"    gl_FragColor = outColor * vec4( color * vColor.xyz, 1.0 );"+
"    float depth = gl_FragCoord.z / gl_FragCoord.w;"+
"    const vec3 fogColor = vec3( 0.0 );"+
"    float fogFactor = smoothstep( 200.0, 600.0, depth );"+
"    gl_FragColor = mix( gl_FragColor, vec4( fogColor, gl_FragColor.w ), fogFactor );"+
"}";



		var renderer, scene, camera, stats;

		var object, uniforms, attributes;

		var vc1;

		var WIDTH = window.innerWidth;
		var HEIGHT = window.innerHeight;

		init();
		animate();

		function init() {

			camera = new THREE.PerspectiveCamera( 40, WIDTH / HEIGHT, 1, 1000 );
			camera.position.z = 500;

			scene = new THREE.Scene();

			attributes = {

				size: {	type: 'f', value: [] },
				ca:   {	type: 'c', value: [] }

			};

			uniforms = {

				amplitude: { type: "f", value: 1.0 },
				color:     { type: "c", value: new THREE.Color( 0xffffff ) },
				texture:   { type: "t", value: THREE.ImageUtils.loadTexture( "http://f.z.nf/ball.png" ) },

			};

			uniforms.texture.value.wrapS = uniforms.texture.value.wrapT = THREE.RepeatWrapping;

			var shaderMaterial = new THREE.ShaderMaterial( {
				uniforms: 		uniforms,
				attributes:     attributes,
				vertexShader:   VERTEXSHADER,
				fragmentShader: FRAGMENTSHADER

			});


			var radius = 100, inner = 0.6 * radius;
			var geometry = new THREE.Geometry();

			for ( var i = 0; i < 100000; i ++ ) {

				var vertex = new THREE.Vector3();
				vertex.x = Math.random() * 2 - 1;
				vertex.y = Math.random() * 2 - 1;
				vertex.z = Math.random() * 2 - 1;
				vertex.multiplyScalar( radius );

				if ( ( vertex.x > inner || vertex.x < -inner ) ||
				     ( vertex.y > inner || vertex.y < -inner ) ||
				     ( vertex.z > inner || vertex.z < -inner )  )

				geometry.vertices.push( vertex );

			}

			vc1 = geometry.vertices.length;

			var m, dummyMaterial = new THREE.MeshFaceMaterial();

			radius = 200;
			var geometry2 = new THREE.CubeGeometry( radius, 0.1 * radius, 0.1 * radius, 50, 5, 5 );

			function addGeo( geo, x, y, z, ry ) {

				m = new THREE.Mesh( geo, dummyMaterial );
				m.position.set( x, y, z );
				m.rotation.y = ry;

				THREE.GeometryUtils.merge( geometry, m );

			}

			// side 1

			addGeo( geometry2, 0,  110,  110, 0 );
			addGeo( geometry2, 0,  110, -110, 0 );
			addGeo( geometry2, 0, -110,  110, 0 );
			addGeo( geometry2, 0, -110, -110, 0 );

			// side 2

			addGeo( geometry2,  110,  110, 0, Math.PI/2 );
			addGeo( geometry2,  110, -110, 0, Math.PI/2 );
			addGeo( geometry2, -110,  110, 0, Math.PI/2 );
			addGeo( geometry2, -110, -110, 0, Math.PI/2 );

			// corner edges

			var geometry3 = new THREE.CubeGeometry( 0.1 * radius, radius * 1.2, 0.1 * radius, 5, 60, 5 );

			addGeo( geometry3,  110, 0,  110, 0 );
			addGeo( geometry3,  110, 0, -110, 0 );
			addGeo( geometry3, -110, 0,  110, 0 );
			addGeo( geometry3, -110, 0, -110, 0 );

			// particle system

			object = new THREE.ParticleSystem( geometry, shaderMaterial );
			object.dynamic = true;

			// custom attributes

			var vertices = object.geometry.vertices;

			var values_size = attributes.size.value;
			var values_color = attributes.ca.value;

			for( var v = 0; v < vertices.length; v ++ ) {

				values_size[ v ] = 10;
				values_color[ v ] = new THREE.Color( 0xffffff );

				if ( v < vc1 ) {

					values_color[ v ].setHSV( 0.5 + 0.2 * ( v / vc1 ), 0.99, 1.0 );

				} else {

					values_size[ v ] = 55;
					values_color[ v ].setHSV( 0.1, 0.99, 1.0 );

				}

			}

			//console.log( vertices.length );

			scene.add( object );

			renderer = new THREE.WebGLRenderer( { clearColor: 0x000000, clearAlpha: 1 } );
			renderer.setSize( WIDTH, HEIGHT );



		}

		function onWindowResize() {

			camera.aspect = window.innerWidth / window.innerHeight;
			camera.updateProjectionMatrix();

			renderer.setSize( window.innerWidth, window.innerHeight );

		}

		function animate() {

			canvas.requestAnimationFrame( animate );

			render();

		}

		function render() {

			var time = Date.now() * 0.01;

			object.rotation.y = object.rotation.z = 0.02 * time;

			for( var i = 0; i < attributes.size.value.length; i ++ ) {

				if ( i < vc1 )
					attributes.size.value[ i ] = Math.max(0, 26 + 32 * Math.sin( 0.1 * i + 0.6 * time ));


			}

			attributes.size.needsUpdate = true;

			renderer.render( scene, camera );

		}


