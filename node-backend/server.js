// const http = require('http');

// const hostname = '127.0.0.1';
// const port = 3000;

// const server = http.createServer((req, res) => {
//   res.statusCode = 200;
//   res.setHeader('Content-Type', 'text/plain');
//   res.end('Hello World');
// });

// server.listen(port, hostname, () => {
//   console.log(`Server running at http://${hostname}:${port}/`);
// });


// const net = require('net');
// const {PromiseSocket} = require("promise-socket")


// // const socket = new net.Socket();
// const promiseSocket = new PromiseSocket();

// (async () => {
//     try {
//         await promiseSocket.connect({ host: '192.168.20.241', port: 20060 });
    
//         console.log("CONNECTED");
//         setTimeout(function() {console.log("done!!")}, 3000);

//         await promiseSocket.write('*SEPOWR################\n');
//         console.log("SNET") 
//         const content = await promiseSocket.readAll() ;
//         console.log("DATA") 
//         console.log(content.toString());
//         //   setTimeout(console.log)
//         // setTimeout(function() {console.log("done!!")}, 3000);
//     }
//     catch (err) {
//         console.log(err)
//     }

//     //   client.end();
    
// })();

const net = require('net');

const client = net.createConnection({ host: '192.168.20.241', port: 20060, family: 'IPv4' }, () => {
  // 'connect' listener.
  console.log('connected to server!');
  client.write('*SEPOWR################\n');
});

client.on('data', (data) => {
  console.log(data.toString());
//   setTimeout(console.log)
  setTimeout(function() {console.log("done!!")}, 3000);
//   client.end();
});

client.on('end', () => {
  console.log('disconnected from server');
});