const char index_html[] = R"rawliteral(
<!DOCTYPE HTML>
<html>

<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <style>
    body {
      font-family: Arial, sans-serif;
      min-width: 310px;
      max-width: 800px;
      margin: 0 auto;
      text-align: center;
      color: #333;
    }

    h2 {
      font-size: 2.5rem;
      margin-bottom: 20px;
    }

    .container {
      height: 400px;
      margin-bottom: 20px;
    }

    .control-panel {
      display: flex;
      justify-content: center;
      align-items: center;
      margin-bottom: 20px;
    }

    .control-item {
      margin-right: 20px;
    }

    .control-item:last-child {
      margin-right: 0;
    }

    .output-switch label {
      margin-right: 10px;
    }

    select,
    button {
      padding: 10px;
      margin-right: 10px;
      border-radius: 5px;
      border: 1px solid #ccc;
      background-color: white;
      font-size: 1rem;
    }

    button {
      cursor: pointer;
      background-color: #059e8a;
      color: white;
      border: none;
    }

    button:hover {
      background-color: #04877a;
    }

    p {
      font-size: 1.2rem;
    }

    #currentVoltage {
      font-weight: bold;
    }

    .switch {
      position: relative;
      display: inline-block;
      width: 60px;
      height: 34px;
    }

    .switch input {
      opacity: 0;
      width: 0;
      height: 0;
    }

    .slider {
      position: absolute;
      cursor: pointer;
      top: 0;
      left: 0;
      right: 0;
      bottom: 0;
      background-color: #ccc;
      transition: .4s;
    }

    .slider:before {
      position: absolute;
      content: "";
      height: 26px;
      width: 26px;
      left: 4px;
      bottom: 4px;
      background-color: white;
      transition: .4s;
    }

    input:checked+.slider {
      background-color: #2196F3;
    }

    input:focus+.slider {
      box-shadow: 0 0 1px #2196F3;
    }

    input:checked+.slider:before {
      transform: translateX(26px);
    }

    .slider.round {
      border-radius: 34px;
    }

    .slider.round:before {
      border-radius: 50%;
    }
  </style>
</head>

<body>
  <h2>Spark Analyzer Web App</h2>
  <div id="chart-current" class="container"></div>
  <div class="voltage-selector">
    <label for="voltage">Select Voltage:</label>
    <select id="voltage">
      <option value="5">5V</option>
      <option value="9">9V</option>
      <option value="12">12V</option>
      <option value="15">15V</option>
      <option value="20">20V</option>
    </select>
    <button onclick="setVoltage()">Set Voltage</button>
  </div>
  <p>Current Voltage: <span id="currentVoltage"></span></p>

  <div class="output-switch">
    <label>Output: </label>
    <label class="switch">
      <input type="checkbox" id="outputToggle" onchange="setOutput()">
      <span class="slider round"></span>
    </label>
  </div>

  </div>
  </div>

  <script>
    var chartC = new Highcharts.Chart({
      chart: { renderTo: 'chart-current' },
      title: { text: 'Current (mA)' },
      series: [{
        showInLegend: false,
        data: []
      }],
      plotOptions: {
        line: { animation: false, dataLabels: { enabled: true } },
        series: { color: '#059e8a' }
      },
      xAxis: {
        type: 'datetime',
        dateTimeLabelFormats: { second: '%H:%M:%S' }
      },
      yAxis: {
        title: { text: 'Current (mA)' },
        // min: -100,
        // max: 2000
      },
      credits: { enabled: false }
    });

    setInterval(function () {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          var x = (new Date()).getTime(),
            y = parseFloat(this.responseText);
          if (chartC.series[0].data.length > 60) {
            chartC.series[0].addPoint([x, y], true, true, true);
          } else {
            chartC.series[0].addPoint([x, y], true, false, true);
          }
        }
      };
      xhttp.open("GET", "/current", true);
      xhttp.send();
    }, 500); //Set sample rate

    function setVoltage() {
      var xhttp = new XMLHttpRequest();
      var voltage = document.getElementById("voltage").value;
      xhttp.open("GET", "/set_voltage?voltage=" + voltage, true);
      xhttp.send();
    }

    function updateVoltageDisplay() {
      var xhttp = new XMLHttpRequest();
      xhttp.onreadystatechange = function () {
        if (this.readyState == 4 && this.status == 200) {
          document.getElementById("currentVoltage").innerText = this.responseText + "V";
        }
      };
      xhttp.open("GET", "/get_voltage", true);
      xhttp.send();
    }
    setInterval(updateVoltageDisplay, 1000); // Update every second
    function setOutput() {
      var xhttp = new XMLHttpRequest();
      var outputState = document.getElementById("outputToggle").checked ? "1" : "0";
      xhttp.open("GET", "/set_output?output=" + outputState, true);
      xhttp.send();
    }

  </script>
</body>

</html>

)rawliteral";