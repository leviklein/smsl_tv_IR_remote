const net = require('net');
let {PythonShell} = require('python-shell');
const storage = require('node-persist');
const Mutex = require('async-mutex').Mutex;
require('log-timestamp');

const mutex = new Mutex();
const python_mutex = new Mutex();

const HOST = '192.168.2.31';
const PORT = 20060;
const PING_MSG = '*SEPOWR################\n';
const BASE_COMMAND = ['-p', '--gap', '105', '-g', '17', '-f', '/etc/tv_smsl/smsl_ir_codes'];

var NO_PYTHON = false;
var MANUAL_TURNOFF = false;

const repeatElement = (element, count) =>
    Array(count).fill(element)

var myArgs = process.argv
if (myArgs.length > 2) {
  var extra_args = myArgs.slice(2);
  console.log('myArgs: ', extra_args);
  
  if(extra_args.includes('debug')) {
    NO_PYTHON = true 
  }
}

async function init_db() {
  await storage.init( /* options ... */ );
  let volume = await storage.getItem('volume');
  if(volume == null) {
    await storage.setItem('volume', 0);
  }
  let powered_on = await storage.getItem('powered_on');
  if(powered_on == null) {
    await storage.setItem('powered_on', false);
  }
}

async function get_data(key) {
  const res = await storage.getItem(key);
  return res;
}

async function set_data(key, value) {
  await storage.setItem(key, value);
  // console.log('DB - setting %s to %s', key, value);
}

function process_tv_message(data) {
  data = data.trim();
  console.log(data);

  if (data.includes('SNVOLU')) {
    process_volume_message(data);
  }
  else if (data.includes('SNAMUT')) {
    process_mute_message(data);
  }
  else if (!data.includes('FFFF') &&
    (data.includes('SNPOWR') || data.includes('SAPOWR'))) {
    process_power_message(data);
  }
}

async function process_volume_message(data) {
  await mutex.runExclusive(async () => {
    const db_volume = await get_data('volume');
    const tv_volume = parseInt(data.slice(-3));
    let new_volume = Math.floor(tv_volume / 2);
    let delta = new_volume - db_volume;

    let is_on = await get_data('powered_on');

    if (is_on) {
      console.log("TV volume: %s, amp_volume: %s", tv_volume, new_volume);
      volume_change(delta);
      io.emit('volume change', new_volume);
      await set_data('volume', new_volume);
    }
  });
}

async function process_mute_message(data) {
  await mutex.runExclusive(async () => {
    const mute = parseInt(data.slice(-1));

    if(mute) {
      console.log("Mute")
      volume_change(-5)
    }
    else {
      console.log("Unmute")
    }
  });
}

async function process_power_message(data) {
  await mutex.runExclusive(async () => {
    const amp_power_state = await get_data('powered_on');
    const tv_power_state = parseInt(data.slice(-1));

    if(MANUAL_TURNOFF == true) {
      if(data.includes('SNPOWR') && tv_power_state == 0){
        MANUAL_TURNOFF = false;
      }
    }
    else {
      if(tv_power_state != amp_power_state) {
        await power_change(!amp_power_state);
      }
    }
  });
}

async function power_change(power_state) {
  command_list = BASE_COMMAND.concat(["key_power"])
  run_python_script(command_list);
  
  io.emit('power change', power_state);
  await set_data('powered_on', power_state);
  
  if(power_state) {
    console.log('Powering on..')
    await new Promise(r => setTimeout(r, 3000));
    console.log('Powered on')
  }
  else {
    console.log('Power off')
  }
}

function volume_change(count) {
  // console.log("Volume change, %s", count)
  let command_list = []
  if(count > 0)
    command_list = BASE_COMMAND.concat(repeatElement("key_up", count))
  else if(count < 0) 
    command_list = BASE_COMMAND.concat(repeatElement("key_down", Math.abs(count)))
  else
    return
    
  // console.log(command_list);
  run_python_script(command_list);
}

async function run_python_script(arg_list) {
  const release = await python_mutex.acquire();
  try {
    let options = {
      mode: 'text',
      pythonOptions: ['-u'], // get print results in real-time
      scriptPath: '/usr/local/bin',
      args: arg_list
    };

    if (NO_PYTHON == true) {
      console.log("script placeholder. start");
      console.log(arg_list);
      release();
      console.log("script placeholder. end");
    }
    else {
      let shell = new PythonShell('irrp.py', options, function (err) {
        if (err) throw err;
      });
  
      shell.on('close', function() {
        release();
      });
    }
  } finally {
    // console.log("done!")
  }
}

init_db();

const client = net.createConnection({ host: HOST, port: PORT, family: 'IPv4' });
client.setKeepAlive(true, 1000*60*2);

client.on('connect', (data) => {
  console.log('connected to server!');
  client.write(PING_MSG);
});

client.on('data', (data) => {
  process_tv_message(data.toString());
});

client.on('end', () => {
  setTimeout(10000, function() {
    console.warn('disconnected from server. reconnecting..');
    client.connect();
  });
});

setInterval(() => {
    client.write(PING_MSG);
  }, 1000*60*5);

// exports.power_change = power_change;
// exports.volume_change = volume_change;
// exports.get_db_data = get_data;



const express = require('express');
const app = express();
const http = require('http').createServer(app);
const path = require('path');

const io = require('socket.io')(http);

const port = 3000

app.get('/api/volume', async (req, res) => {
  result = await get_data('volume')
  res.send(result.toString());
});

app.get('/api/volume/up', async (req, res) => {
    console.log("Manually increase amplifier volume");
    volume_change(1);
  res.sendStatus(204);
});

app.get('/api/volume/down', async (req, res) => {
    console.log("Manually decrease amplifier volume");
    volume_change(-1);
  res.sendStatus(204);
});

app.get('/api/power/toggle', async (req, res) => {
  let power = await get_data('powered_on');
  if (power) {
    console.log("Manually turning off amplifier");
    MANUAL_TURNOFF = true;
    await power_change(!power);
  }
  else if (MANUAL_TURNOFF) {
    console.log("Manually turning on amplifier");
    MANUAL_TURNOFF = false;
    await power_change(!power);
  }
  res.sendStatus(204);
});

app.get('/api/power', async (req, res) => {
  result = await get_data('powered_on')
  res.send(result.toString());
});

app.get('/api/power/on', async (req, res) => {
  await set_data('powered_on', true);
  io.emit('power change', true);
  res.sendStatus(204);
});

app.get('/api/power/off', async (req, res) => {
  await set_data('powered_on', false);
  io.emit('power change', false);
  res.sendStatus(204);
});

app.use(express.static('html'))

io.on('connection', async (socket) => {
  console.log('a client connected');
  socket.emit('power change', await get_data('powered_on'))
  socket.emit('volume change', await get_data('volume'))
});


http.listen(port, () => {
  console.log(`App listening at http://localhost:${port}`)
})