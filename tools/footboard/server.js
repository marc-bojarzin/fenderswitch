// fenderswitch footboard: a web page with footswitch buttons that sends MIDI
// Program Change to the Nano over its USB serial port.
//
// The Nano's USB chip is a USB-serial converter, so the server writes the raw
// MIDI bytes to the serial port at 31250 baud. The firmware reads them like
// MIDI from the DIN socket. The server also reads the firmware's Serial output
// and pushes it to the page (Server-Sent Events), so the page shows the
// program and channel the device actually switched to.
//
// Settings (environment variables):
//   SERIAL_PORT   serial port of the Nano     (default /dev/ttyUSB0)
//   MIDI_CHANNEL  MIDI channel 1-16 to send   (default 1)
//   PORT          HTTP port                   (default 8080)
//   HOST          address to listen on        (default 0.0.0.0, i.e. also
//                                              reachable from a phone or
//                                              tablet on the same network)

import http from 'node:http';
import { readFile } from 'node:fs/promises';
import { fileURLToPath } from 'node:url';
import path from 'node:path';
import { SerialPort, ReadlineParser } from 'serialport';

const SERIAL_PORT = process.env.SERIAL_PORT ?? '/dev/ttyUSB0';
const MIDI_CHANNEL = Number(process.env.MIDI_CHANNEL ?? 1);
const HTTP_PORT = Number(process.env.PORT ?? 8080);
const HOST = process.env.HOST ?? '0.0.0.0';

const MIDI_BAUD = 31250;
const PROGRAM_COUNT = 128;

// Opening the port resets the Nano (DTR auto-reset). Bytes sent while the
// bootloader runs are lost, so sending waits this long after opening.
const BOOT_WAIT_MS = 2500;
// Retry interval while the port is missing (Nano unplugged).
const RETRY_MS = 2000;

const PUBLIC_DIR = path.join(path.dirname(fileURLToPath(import.meta.url)), 'public');

if (!(MIDI_CHANNEL >= 1 && MIDI_CHANNEL <= 16)) {
  console.error(`MIDI_CHANNEL must be 1-16, got ${process.env.MIDI_CHANNEL}`);
  process.exit(1);
}

// What the page shows. program/channel come from the firmware's output, so
// they reflect the device, not just what was sent.
const state = {
  connected: false,   // port open and the Nano done booting
  program: null,      // 1-128
  ampChannel: null,   // "Clean", "Drive" or "More Drive"
  modified: false,    // preset changed with a channel button, not stored
};

// ---------------------------------------------------------------------------
// Server-Sent Events: every open page gets state changes and log lines.

const clients = new Set();

function broadcast(event, data) {
  const message = `event: ${event}\ndata: ${JSON.stringify(data)}\n\n`;
  for (const res of clients) {
    res.write(message);
  }
}

function setState(changes) {
  Object.assign(state, changes);
  broadcast('state', state);
}

// ---------------------------------------------------------------------------
// Serial port.

let port = null;

// Parses one line of the firmware's Serial output into state changes.
//   program=12 channel=Drive (modified)
//   stored program=12 channel=Drive
function handleLine(line) {
  broadcast('log', { dir: 'in', text: line });
  const match = /^(?:stored )?program=(\d+) channel=(Clean|Drive|More Drive)( \(modified\))?$/.exec(line);
  if (match) {
    setState({
      program: Number(match[1]),
      ampChannel: match[2],
      modified: Boolean(match[3]),
    });
  }
}

function openPort() {
  const p = new SerialPort({ path: SERIAL_PORT, baudRate: MIDI_BAUD, autoOpen: false });
  p.open((err) => {
    if (err) {
      console.error(`serial: ${err.message}, retrying in ${RETRY_MS / 1000} s`);
      setTimeout(openPort, RETRY_MS);
      return;
    }
    console.log(`serial: ${SERIAL_PORT} open at ${MIDI_BAUD} baud, waiting for the Nano to boot`);
    port = p;
    p.pipe(new ReadlineParser({ delimiter: '\n' }))
      .on('data', (line) => handleLine(line.replace(/\r$/, '')));
    setTimeout(() => {
      if (port === p) {
        console.log('serial: ready');
        setState({ connected: true });
      }
    }, BOOT_WAIT_MS);
  });
  p.on('close', () => {
    console.error(`serial: ${SERIAL_PORT} closed, retrying in ${RETRY_MS / 1000} s`);
    port = null;
    setState({ connected: false });
    setTimeout(openPort, RETRY_MS);
  });
  p.on('error', (err) => console.error(`serial: ${err.message}`));
}

// Sends Program Change for program 1-128 on MIDI_CHANNEL.
function sendProgramChange(program) {
  const bytes = [0xC0 | (MIDI_CHANNEL - 1), program - 1];
  port.write(Buffer.from(bytes));
  broadcast('log', {
    dir: 'out',
    text: `program change ch=${MIDI_CHANNEL} pc=${program} [${bytes.map((b) => b.toString(16).padStart(2, '0')).join(' ')}]`,
  });
}

// ---------------------------------------------------------------------------
// HTTP.

const CONTENT_TYPES = {
  '.html': 'text/html; charset=utf-8',
  '.js': 'text/javascript; charset=utf-8',
  '.css': 'text/css; charset=utf-8',
  '.svg': 'image/svg+xml',
};

function sendJson(res, status, body) {
  res.writeHead(status, { 'Content-Type': 'application/json' });
  res.end(JSON.stringify(body));
}

async function readJson(req) {
  let body = '';
  for await (const chunk of req) {
    body += chunk;
    if (body.length > 1024) throw new Error('body too large');
  }
  return JSON.parse(body);
}

async function handleRequest(req, res) {
  const url = new URL(req.url, 'http://localhost');

  if (req.method === 'POST' && url.pathname === '/api/program') {
    let program;
    try {
      ({ program } = await readJson(req));
    } catch {
      return sendJson(res, 400, { error: 'expected JSON {"program": 1-128}' });
    }
    if (!Number.isInteger(program) || program < 1 || program > PROGRAM_COUNT) {
      return sendJson(res, 400, { error: 'program must be an integer 1-128' });
    }
    if (!state.connected) {
      return sendJson(res, 503, { error: `${SERIAL_PORT} not connected` });
    }
    sendProgramChange(program);
    return sendJson(res, 200, { ok: true, program });
  }

  if (req.method === 'GET' && url.pathname === '/api/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      Connection: 'keep-alive',
    });
    res.write(`event: config\ndata: ${JSON.stringify({ midiChannel: MIDI_CHANNEL, serialPort: SERIAL_PORT })}\n\n`);
    res.write(`event: state\ndata: ${JSON.stringify(state)}\n\n`);
    clients.add(res);
    req.on('close', () => clients.delete(res));
    return;
  }

  if (req.method === 'GET') {
    const name = url.pathname === '/' ? 'index.html' : url.pathname.slice(1);
    const file = path.join(PUBLIC_DIR, name);
    // Only files directly inside public/.
    if (path.dirname(file) !== PUBLIC_DIR) {
      res.writeHead(404).end();
      return;
    }
    try {
      const content = await readFile(file);
      res.writeHead(200, { 'Content-Type': CONTENT_TYPES[path.extname(file)] ?? 'application/octet-stream' });
      res.end(content);
    } catch {
      res.writeHead(404).end();
    }
    return;
  }

  res.writeHead(405).end();
}

// Keeps idle SSE connections alive through proxies and sleeping phones.
setInterval(() => {
  for (const res of clients) res.write(': ping\n\n');
}, 15000);

http.createServer((req, res) => {
  handleRequest(req, res).catch((err) => {
    console.error(err);
    if (!res.headersSent) res.writeHead(500);
    res.end();
  });
}).listen(HTTP_PORT, HOST, () => {
  console.log(`footboard: http://localhost:${HTTP_PORT}/ (MIDI channel ${MIDI_CHANNEL})`);
});

openPort();
