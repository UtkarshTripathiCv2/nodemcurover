#include <Wire.h>
#include <Adafruit_PWMServoDriver.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>

Adafruit_PWMServoDriver pca9685 = Adafruit_PWMServoDriver(0x40);
ESP8266WebServer server(80);

String correctUser = "admin";
String correctPass = "sofcon";
bool authenticated = false;


// Motor pin mapping
#define IN1 D3
#define IN2 D4
#define IN3 D5
#define IN4 D6

// Servo limits
#define SERVOMIN  150
#define SERVOMAX  600

// WiFi credentials
const char* ssid = "Sofcon Noida_5G";;
const char* password = "sipl_n2@";


void setup() {
  Serial.begin(9600);
  Wire.begin(D2, D1);  // SDA, SCL
  pca9685.begin();
  pca9685.setPWMFreq(50);

  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  stopMotors();

  // --- WiFi setup ---
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.print("\nConnecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nConnected!");
  Serial.print("Access dashboard at: http://");
  Serial.println(WiFi.localIP());

  // --- Web routes ---
  server.on("/", handleLogin);          // Default → login
  server.on("/login", handleLogin);     // Handles login form
  server.on("/dashboard", handleDashboard); // Secure dashboard
  server.on("/servo", handleServo);     // Servo control
  server.on("/move", handleMove);       // Motor control

  // ✅ IMPORTANT: Start the web server
  server.begin();
  Serial.println("HTTP server started successfully.");
}

void loop() {
  server.handleClient();
}

// -------------------- Servo Handler --------------------
void handleServo() {
  if (!server.hasArg("id") || !server.hasArg("val")) {
    server.send(400, "text/plain", "Missing parameters");
    return;
  }

  int id = server.arg("id").toInt();
  int val = server.arg("val").toInt();
  val = constrain(val, 0, 180);

  int pwm = map(val, 0, 180, SERVOMIN, SERVOMAX);
  pca9685.setPWM(id, 0, pwm);

  Serial.printf("Servo %d -> %d° (PWM: %d)\n", id, val, pwm);
  server.send(200, "text/plain", "OK");
}

// -------------------- Motor Handler --------------------
void handleMove() {
  if (!server.hasArg("dir")) {
    server.send(400, "text/plain", "Missing direction");
    return;
  }

  String dir = server.arg("dir");
  dir.toUpperCase();

  if (dir == "F") forward();
  else if (dir == "B") backward();
  else if (dir == "L") left();
  else if (dir == "R") right();
  else stopMotors();

  Serial.println("Direction: " + dir);
  server.send(200, "text/plain", "OK");
}

// -------------------- Motor Control Functions --------------------
void forward() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void backward() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void left() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
}
void right() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
}
void stopMotors() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}

void handleDashboard() {
  if (!authenticated) {
    server.sendHeader("Location", "/");
    server.send(303);
    return;
  }

  String html = R"=====( 
  <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Sofcon India - Robot Control</title>
  <style>
    body{
      margin:0;
      background:#ffffff;
      font-family:'Segoe UI',sans-serif;
      color:#000;
      text-align:center;
    }
    header{
      background:#0078d4;
      color:white;
      padding:10px 0;
      font-size:22px;
      font-weight:600;
      box-shadow:0 2px 6px rgba(0,0,0,0.2);
    }
    h2{
      color:#0078d4;
      margin:20px 0;
      font-weight:600;
    }
    .card{
      background:white;
      border-radius:15px;
      border:2px solid #0078d4;
      max-width:700px;
      margin:20px auto;
      padding:25px;
      box-shadow:0 4px 12px rgba(0,0,0,0.1);
    }
    .container{
      display:grid;
      grid-template-columns:repeat(2,1fr);
      gap:30px;
      justify-content:center;
    }
    .gaugeBox{
      text-align:center;
    }
    .gauge{
      width:180px;
      height:90px;
      position:relative;
      margin:auto;
    }
    .arc{
      width:100%;
      height:100%;
      background:none;
      border:15px solid #ddd;
      border-bottom:none;
      border-radius:180px 180px 0 0;
      position:relative;
      overflow:hidden;
    }
    .fill{
      position:absolute;
      top:0;
      left:0;
      width:100%;
      height:100%;
      background:none;
      border:15px solid #0078d4;
      border-bottom:none;
      border-radius:180px 180px 0 0;
      clip-path:polygon(0% 100%,0% 100%,50% 100%,100% 100%,100% 100%);
      transform-origin:center bottom;
      transform:rotate(0deg);
      transition:transform 0.4s ease;
    }
    .val{
      position:absolute;
      top:45%;
      left:50%;
      transform:translate(-50%,-50%);
      font-size:20px;
      font-weight:bold;
    }
    .range-labels{
      display:flex;
      justify-content:space-between;
      margin-top:-5px;
      font-size:13px;
      color:#555;
    }
    .label{
      margin-bottom:8px;
      font-weight:500;
    }
    .btns{
      display:grid;
      grid-template-columns:repeat(3,1fr);
      max-width:350px;
      margin:25px auto;
    }
    button{
      padding:15px;
      margin:8px;
      border:none;
      border-radius:8px;
      background:#0078d4;
      color:white;
      font-size:16px;
      cursor:pointer;
    }
    button:hover{background:#005ea3;}
  </style>
  </head>
  <body>
  <header>Sofcon India Private Ltd.</header>
  <div class="card">
    <h2>NodeMCU Servo & Motor Dashboard</h2>

    <div class="container">
      <div class="gaugeBox" data-id="0">
        <div class="label">Servo_1</div>
        <div class="gauge">
          <div class="arc"></div>
          <div class="fill" id="f1"></div>
          <div class="val" id="v1">0</div>
        </div>
        <div class="range-labels"><span>0</span><span>180</span></div>
      </div>
      <div class="gaugeBox" data-id="1">
        <div class="label">Servo_2</div>
        <div class="gauge">
          <div class="arc"></div>
          <div class="fill" id="f2"></div>
          <div class="val" id="v2">0</div>
        </div>
        <div class="range-labels"><span>0</span><span>180</span></div>
      </div>
      <div class="gaugeBox" data-id="2">
        <div class="label">Servo_3</div>
        <div class="gauge">
          <div class="arc"></div>
          <div class="fill" id="f3"></div>
          <div class="val" id="v3">0</div>
        </div>
        <div class="range-labels"><span>0</span><span>180</span></div>
      </div>
      <div class="gaugeBox" data-id="3">
        <div class="label">Servo_4</div>
        <div class="gauge">
          <div class="arc"></div>
          <div class="fill" id="f4"></div>
          <div class="val" id="v4">0</div>
        </div>
        <div class="range-labels"><span>0</span><span>180</span></div>
      </div>
    </div>

    <div class='btns'>
      <div></div><button onclick="move('F')">&#8593; Forward</button><div></div>
      <button onclick="move('L')">&#8592; Left</button>
      <button onclick="move('S')">&#9632; Stop</button>
      <button onclick="move('R')">&#8594; Right</button>
      <div></div><button onclick="move('B')">&#8595; Backward</button><div></div>
    </div>
  </div>

  <script>
    const boxes=document.querySelectorAll('.gaugeBox');
    boxes.forEach(b=>{
      b.addEventListener('mousedown',e=>handle(e,b));
      b.addEventListener('mousemove',e=>{if(e.buttons===1)handle(e,b);});
      b.addEventListener('touchmove',e=>handle(e.touches[0],b));
    });

    function handle(e,b){
      const r=b.querySelector('.gauge').getBoundingClientRect();
      const cx=r.left+r.width/2,cy=r.bottom; // bottom center for half circle
      const dx=e.clientX-cx,dy=e.clientY-cy;
      let angle=Math.atan2(-dy,dx)*180/Math.PI;
      if(angle<0)angle=0;if(angle>180)angle=180;
      updateGauge(b.dataset.id,Math.round(angle));
    }

    function updateGauge(id,val){
      fetch(/servo?id=${id}&val=${val});
      const f=document.getElementById(f${+id+1});
      const v=document.getElementById(v${+id+1});
      f.style.transform=rotate(${val-90}deg); // animate arc fill
      v.textContent=val;
    }

    function move(dir){fetch(/move?dir=${dir});}
  </script>
  </body></html>
  )=====";
  server.send(200,"text/html",html);
}

// -------------------- Login Page --------------------
void handleLogin() {
  if (server.hasArg("username") && server.hasArg("password")) {
    String u = server.arg("username");
    String p = server.arg("password");

    if (u == correctUser && p == correctPass) {
      authenticated = true;
      server.sendHeader("Location", "/dashboard");
      server.send(303);
      return;
    }
  }

  String html = R"=====(
  <!DOCTYPE html>
  <html lang="en">
  <head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Login - Sofcon Dashboard</title>
  <style>
    body{
      margin:0;
      background:#f9f9f9;
      font-family:'Segoe UI',sans-serif;
      display:flex;
      justify-content:center;
      align-items:center;
      height:100vh;
    }
    .card{
      background:white;
      padding:40px;
      border-radius:12px;
      box-shadow:0 0 15px rgba(0,0,0,0.1);
      text-align:center;
      width:300px;
    }
    h2{color:#0078d4;margin-bottom:25px;}
    input{
      width:100%;
      padding:10px;
      margin:8px 0;
      border:1px solid #ccc;
      border-radius:6px;
    }
    button{
      background:#0078d4;
      color:white;
      border:none;
      padding:10px 20px;
      border-radius:6px;
      cursor:pointer;
      width:100%;
      margin-top:10px;
    }
    button:hover{background:#005ea3;}
  </style>
  </head>
  <body>
  <div class="card">
    <h2>Login</h2>
    <form action="/login" method="POST">
      <input type="text" name="username" placeholder="Username" required><br>
      <input type="password" name="password" placeholder="Password" required><br>
      <button type="submit">Login</button>
    </form>
  </div>
  </body></html>
  )=====";
  server.send(200, "text/html", html);
}
