const dgram = require("dgram");

const frameRate = 120;

setInterval(sendPacket, 1000 / frameRate);

const socket = dgram.createSocket("udp4");

const buffer = new Uint8Array(4096);
buffer.fill(0);
socket.connect(6000, "192.168.17.116");

function sendPacket() {
  socket.send(buffer);
}
