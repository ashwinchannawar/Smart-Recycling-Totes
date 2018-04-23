
AWS.config.update({ region: "us-east-2", endpoint: 'https://dynamodb.us-east-2.amazonaws.com', accessKeyId: "AKIAJJOB4Y6Z245JSBUQ", secretAccessKey: "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"});
var db = new AWS.DynamoDB.DocumentClient();
var map;
var devices = [
	{
		id: "1C9FBC",
		progress: "#progress-bar1",
		marker: null,
		coords: {
			lat: 43, 
			long: -78.787 // UBNorth coords
		},
		cell: {
			row: 1,
			col: 2
		}
	},
	{
		id: "1C9FC0",
		progress: "#progress-bar2",
		marker: null,
		coords: {
			lat: 42.952, 
			long: -78.819 // UBSouth coords 
		},
		cell: {
			row: 2,
			col: 2
		}
	}
];

function startFunction() {
	initMap(); 
	initMarkers();
	setInterval(updateData, 1000); // run function every 1 sec
}

function initMap() {
	map = new google.maps.Map(document.getElementById('map'), {
		zoom: 12,
		center: new google.maps.LatLng(42.95, -78.8),
	});
}

// create and store markers in device object
function initMarkers() {
	devices.forEach(function(device) { // nicer way to iterate through devices versus for loop
		var marker = new google.maps.Marker({
		position: new google.maps.LatLng(device.coords.lat,device.coords.long),
		map: null // start with markers off
		});
		device.marker = marker;	
	});
}

function updateData(){
	devices.forEach(function(device) {
		var par = [{ // parameters for dynamodb getitem function
				TableName: "testfox",
				Key:{
					"deviceid": device.id
				}
			},
			{
				TableName: "AverageVolume",
				Key:{
					"deviceid": "Total"
				}	
			}];
		// grab data from AWS dynamodb
		db.get(par[0], function(err, data) { 
			updateProgress(data.Item.payload.percent, device.progress); // update progress bar in table
			updateWeight(data.Item.payload.weight, device.cell);        // update weight in table
			updateMarker(data.Item.payload.percent, device);		    // update the markers
		});
		db.get(par[1], function(err, w) { 
			updateChart(w.Item.averageVolume);	//update chart with total volume (not avg)
		});
	});
}

function updateMarker(percentage, device){
	if (percentage >= 75) {
		device.marker.setMap(map);
	}
	else if (percentage < 75)
	{
		device.marker.setMap(null);
	}
}

function updateProgress(value, progressBar) {
	$(progressBar)
	.css("width", value + "%")
	.attr("aria-valuenow", value)
	.text(value + "%");
}

function updateWeight(weight, c){
	document.getElementById('table').rows[c.row].cells[c.col].innerHTML = weight;
}

function updateChart(avg) { 
  chart.config.data.datasets[0].data[6] = avg;
  chart.update();
}

// chart
var ctx = document.getElementById('myChart');
var chart = new Chart(ctx, {
// The type of chart we want to create
	type: 'line',
	// The data for our dataset
	data: {
		labels: ["June", "July", "August", "September", "October", "November", "December"],
		datasets: [{
			borderColor: 'rgb(50, 150, 50)',
			data: [37, 44, 40, 37, 54, 47, 0],
			fill: false
		}],
	},
	// Configuration options 
	options: {
		legend: {
			display: false
		},
		title: {
			display: true,
			text: 'Total Volume Recycled',
			fontSize: 16
		},
		scales: {
			yAxes: [{
				scaleLabel: {
				display: true,
				labelString: 'Volume (gal)'
	      		}
			}]
		}
	}
});