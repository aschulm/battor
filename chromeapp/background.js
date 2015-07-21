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
		console.log("connected!")
		connectionId = connectionInfo.connectionId;
		data = new Uint8Array([0x51, 0x51, 0x51, 0x51, 0x51]);
		data = data.buffer;

		var onSend = function(i) {
			console.log(i);
			chrome.serial.flush(connectionId, function(i) {});
		}
		chrome.serial.send(connectionId, data, function(i) {console.log(i)});
	}
	chrome.serial.connect('/dev/ttyUSB0', {bitrate: 2000000}, onConnect);
});
