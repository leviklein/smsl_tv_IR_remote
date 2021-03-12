const net = require('net');
let {PythonShell} = require('python-shell');
const storage = require('node-persist');
var Mutex = require('async-mutex').Mutex;

const mutex = new Mutex();

const HOST = '192.168.20.241'
const PORT = 20060 
const PING_MSG = '*SEPOWR################\n'
const BASE_COMMAND = ['/usr/local/bin/irrp.py', '-p', '-g', '17', '-f', '/etc/tv_smsl/smsl_ir_codes']


async function init_db() {
  await storage.init( /* options ... */ );
  let value = await storage.getItem('volume');
  if(value == null) {
    await storage.setItem('volume', 0);
  }
}

init_db();

async function get_data() {
  const res = await storage.getItem('volume');
  return res
}

async function set_data(volume) {
    // ...
    await storage.setItem('volume',volume);
    console.log(volume)
}

async function set_volume() {
  await mutex.runExclusive(async () => {
    data = await get_data();
    set_data(data+5);
  });
}


function process_tv_message(data) {
  // set_volume();

  // let shell = PythonShell.run('../test_locking.py', null, function (err) {
  // if (err) throw err;
  // console.log('finished');
  // });

  if (data.includes('SNVOLU')) {

  }
  else if  (data.includes('SNAMUT')) {

  }
  else if  (data.includes('SNPOWR')) {

  }
}

function volume_change(data) {
  console.log("Volume change!")

  tv_volume = int(data[-3:])
  new_volume = tv_volume // 2 # max 50 only
  delta_volume = new_volume - status["volume"]

}

const client = net.createConnection({ host: HOST, port: PORT, family: 'IPv4' }, () => {
  console.log('connected to server!');
  client.write(PING_MSG);
});

client.on('data', (data) => {
  console.log(data.toString());
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