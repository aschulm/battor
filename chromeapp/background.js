chrome.app.runtime.onLaunched.addListener(function() {
  chrome.app.window.create('window.html', {
	'outerBounds': {
	'width': 400,
	'height': 500
	}
	});

	var onGetDevices = function(ports) {
		for (var i=0; i<ports.length; i++) {
			console.log(ports[i].path);
		}
	}
	chrome.serial.getDevices(onGetDevices);	

	var onConnect = function(connectionInfo) {
		connectionId = connectionInfo.conectionId;

		var onSend = function(moo) {
		}
		chrome.serial.send(connectionId, Uint8Array([0x00, 0x00, 0x00]), onSend);
		chrome.serial.flush(connectionId, onFlush);
	}

	chrome.serial.connect("/dev/ttyUSB0", {bitrate: 2000000}, onConnect);
});
