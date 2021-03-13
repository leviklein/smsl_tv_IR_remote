const net = require('net');
let {PythonShell} = require('python-shell');
const storage = require('node-persist');
var Mutex = require('async-mutex').Mutex;

const mutex = new Mutex();

const HOST = '192.168.20.241'
const PORT = 20060 
const PING_MSG = '*SEPOWR################\n'
const BASE_COMMAND = ['-p', '-g', '17', '-f', '/etc/tv_smsl/smsl_ir_codes']

const repeatElement = (element, count) =>
    Array(count).fill(element)

async function init_db() {
  await storage.init( /* options ... */ );
  let value = await storage.getItem('volume');
  if(value == null) {
    await storage.setItem('volume', 0);
  }
}

init_db();

async function get_data(key) {
  const res = await storage.getItem(key);
  return res;
}

async function set_data(key, value) {
  await storage.setItem(key, value);
  console.log('setting %s to %s', key, value);
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
  else if (data.includes('SNPOWR')) {
    process_power_message(data);
  }
}

async function process_volume_message(data) {
  await mutex.runExclusive(async () => {
    const db_volume = await get_data('volume');
    const tv_volume = parseInt(data.slice(-3));
    let new_volume = Math.floor(tv_volume / 2);
    let delta = new_volume - db_volume;

    console.log("TV volume: %s, amp_volume: %s", tv_volume, new_volume);

    volume_change(delta)
    set_data('volume', new_volume)
  });
}

function process_mute_message(data) {
  const mute = parseInt(data.slice(-1));

  if(mute) {
    console.log("Mute")
    volume_change(-5)
  }
  else {
    console.log("Unmute")
  }
}

async function process_power_message(data) {
  await mutex.runExclusive(async () => {
    const amp_power_state = await get_data('powered_on');
    const tv_power_state = parseInt(data.slice(-1));

    if (tv_power_state != amp_power_state) {
      power_change(tv_power_state);
    }
  });
}

async function power_change(power_state) {
  command_list = BASE_COMMAND.concat(["key_power"])
  run_python_script(command_list);
  
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
  console.log("Volume change, %s", count)

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

function run_python_script(arg_list) {
  let options = {
    mode: 'text',
    pythonOptions: ['-u'], // get print results in real-time
    scriptPath: '/usr/local/bin',
    args: arg_list
  };

  // console.log("script placeholder");
  // console.log(arg_list);
  PythonShell.run('irrp.py', options, function (err) {
    if (err) throw err;
  });
}

const client = net.createConnection({ host: HOST, port: PORT, family: 'IPv4' }, () => {
  console.log('connected to server!');
  client.write(PING_MSG);
});

client.on('data', (data) => {
  process_tv_message(data.toString());
});

client.on('end', () => {
  console.log('disconnected from server');
});