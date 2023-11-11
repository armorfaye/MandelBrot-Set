if (Module) {
	Module.onRuntimeInitialized = function() {};
}

function isNumber(x){
	if (isNaN(x)) {
		alert("Please enter a number.");
		return;
	}
}

function displayMandelBrot() {
	var width = parseInt(document.getElementById("width").value, 10);
	var startX = parseFloat(document.getElementById("startX").value);
	var startY = parseFloat(document.getElementById("startY").value);
	var endX = parseFloat(document.getElementById("endX").value);
	var endY = parseFloat(document.getElementById("endY").value);
	isNumber(width); 
	isNumber(startX); 
	isNumber(startY); 
	isNumber(endX); 
	isNumber(endY); 
	try {
		var rgb = Module.genPixels(width, startX, startY, endX, endY);
		const height = rgb.size() / width;

		var canvas = document.getElementById("myCanvas");
		canvas.width = width;
		canvas.height = height;

		var ctx = canvas.getContext('2d');
		var imageData = ctx.createImageData(width, height);

		for (let i = 0; i < rgb.size(); i++) {
			const index = i * 4;
			const color = rgb.get(i);

			imageData.data[index + 0] = color.r;
			imageData.data[index + 1] = color.g;
			imageData.data[index + 2] = color.b;
			imageData.data[index + 3] = 255;
		}
		ctx.putImageData(imageData, 0, 0);
	} catch (e) {
		console.error("An error occurred while generating the Mandelbrot set:", e);
	}
}