const net = require('net');
let {PythonShell} = require('python-shell');
const storage = require('node-persist');
var Mutex = require('async-mutex').Mutex;

const mutex = new Mutex();

const HOST = '192.168.20.241'
const PORT = 20060 
const PING_MSG = '*SEPOWR################\n'
const BASE_COMMAND = ['/usr/local/bin/irrp.py', '-p', '-g', '17', '-f', '/etc/tv_smsl/smsl_ir_codes']

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
  return res
}

async function set_data(key, value) {
    await storage.setItem(key, value);
    console.log(value)
}



function process_tv_message(data) {
  // set_volume();

  // let shell = PythonShell.run('../test_locking.py', null, function (err) {
  // if (err) throw err;
  // console.log('finished');
  // });

  data = data.trim();
  console.log(data);

  if (data.includes('SNVOLU')) {
    process_volume_message(data);
  }
  else if  (data.includes('SNAMUT')) {

  }
  else if  (data.includes('SNPOWR')) {

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

function volume_change(count) {
  console.log("Volume change!")

  let command_list = []

  if(count > 0)
    command_list = BASE_COMMAND.concat(repeatElement("key_up", count))
  else if(count < 0) 
    command_list = BASE_COMMAND.concat(repeatElement("key_down", Math.abs(count)))
  else
    return
    
  console.log(command_list);

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


// let shell = PythonShell.run('../test_locking.py', null, function (err) {
//   if (err) throw err;
//   console.log('finished');
// });

// shell.on('message', function (message) {
//   // handle message (a line of text from stdout)
//   console.log(message)
// });