const ws = new WebSocket('ws://' + location.hostname + '/ws');
const statusEl = document.getElementById('status');
const scanBtn = document.getElementById('scan-btn');
const networksEl = document.getElementById('networks');
const notificationsEl = document.getElementById('notifications');
const sensorsEl = document.getElementById('sensors-container');

// ------------------- WebSocket Handlers --------------------
ws.onopen = () => {
  statusEl.textContent = 'Conectado con ESP32';
  notificationsEl.textContent = 'WebSocket abierto';
};

ws.onerror = (err) => {
  statusEl.textContent = 'Error de conexión';
  notificationsEl.textContent = 'Error en WebSocket';
  console.error(err);
};

ws.onclose = () => {
  statusEl.textContent = 'Conexión cerrada';
  notificationsEl.textContent = 'WebSocket cerrado';
};

scanBtn.onclick = () => {
  ws.send('SCAN_WIFI');
  statusEl.textContent = 'Escaneando redes...';
  notificationsEl.textContent = 'Enviando SCAN_WIFI';
};

ws.onmessage = (event) => {
  try {
    const data = JSON.parse(event.data);
    console.log('data.networks:', data.networks);

    if (data.notifications) {
      notificationsEl.textContent = data.notifications;
    }

    if (Array.isArray(data.networks)) {
      statusEl.textContent = 'Redes disponibles:';
      showNetworks(data.networks);
    }

    if (data.sensors) {
      updateSensorCards(data.sensors);
    }

  } catch (e) {
    notificationsEl.textContent = event.data;
  }
};

// ------------------- Mostrar Redes WiFi --------------------
function showNetworks(networks) {
  networksEl.innerHTML = '';
  networks.forEach((net, index) => {
    const signalBars = getSignalBars(net.rssi);
    const div = document.createElement('div');
    div.className = 'network';

    div.addEventListener('click', (e) => {
      const ignoreTags = ['INPUT', 'BUTTON', 'LABEL'];
      if (!ignoreTags.includes(e.target.tagName)) {
        div.classList.toggle('open');
      }
    });

    div.innerHTML = `
      <div class="network-header">
        <strong>${net.ssid}</strong>
        <span class="signal">${signalBars}</span>
      </div>
      <div class="details">
        <input type="password" placeholder="Contraseña" id="pwd-${index}" />
        <label>
          <input type="checkbox" onclick="event.stopPropagation(); togglePasswordVisibility('pwd-${index}')"> Ver contraseña
        </label>
        <button class="btn btn-connect" onclick="event.stopPropagation(); sendCredentials('${net.ssid}', 'pwd-${index}')">Conectarse</button>
      </div>
    `;
    networksEl.appendChild(div);
  });
}

// ------------------- Señal WiFi Visual --------------------
function getSignalBars(rssi) {
  let bars = 0;
  if (rssi >= -50) bars = 5;
  else if (rssi >= -60) bars = 4;
  else if (rssi >= -70) bars = 3;
  else if (rssi >= -80) bars = 2;
  else if (rssi >= -90) bars = 1;

  let html = '';
  for (let i = 1; i <= 5; i++) {
    html += `<div class="bar ${i <= bars ? 'on' : 'off'}"></div>`;
  }
  return html;
}

function createSensorCards() {
  for (let i = 1; i <= 8; i++) {
    const card = document.createElement('div');
    card.className = 'sensor-card';
    card.id = `sensor-canal${i}`;
    card.innerHTML = `
      <div class="sensor-title" id="sensor-title-canal${i}">Canal ${i}</div>
      <div class="sensor-value" id="sensor-value-canal${i}">--</div>
    `;
    sensorsEl.appendChild(card);
  }
}

function updateSensorCards(sensores) {
  for (let i = 1; i <= 8; i++) {
    const sensorData = sensores[`canal${i}`];
    if (!sensorData) continue;

    const valueEl = document.getElementById(`sensor-value-canal${i}`);
    const titleEl = document.getElementById(`sensor-title-canal${i}`);

    if (sensorData.valor !== undefined && valueEl) {
      valueEl.textContent = sensorData.valor;
    }

    if (sensorData.titulo !== undefined && titleEl) {
      titleEl.textContent = sensorData.titulo;
    }
  }
}

// ------------------- Utilidades --------------------
window.togglePasswordVisibility = (inputId) => {
  const input = document.getElementById(inputId);
  input.type = input.type === 'password' ? 'text' : 'password';
};

window.sendCredentials = (ssid, pwdId) => {
  const pwd = document.getElementById(pwdId).value;
  const payload = { cmd: 'CONNECT_WIFI', ssid: ssid, password: pwd };
  ws.send(JSON.stringify(payload));
  notificationsEl.textContent = `Conectando a ${ssid}`;
};

let networksVisible = true;
function toggleNetworks() {
  networksVisible = !networksVisible;
  document.getElementById('networks').style.display = networksVisible ? 'grid' : 'none';
  document.getElementById('toggle-btn').textContent = networksVisible ? 'Ocultar redes' : 'Mostrar redes';
}

// ------------------- Inicialización --------------------
createSensorCards();