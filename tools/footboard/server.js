// fenderswitch footboard and device monitor.
//
// Two separate web devices share this server, connected only through the
// Nano's USB serial port, like a footboard and the fenderswitch on a MIDI
// cable:
//
//  - Footboard (footboard.html, /api/midi/...): a plain MIDI footboard. It
//    sends Program Change and knows nothing about what the receiver does with
//    it. The Nano's USB chip is a USB-serial converter, so the MIDI bytes are
//    written to the serial port at 31250 baud; the firmware reads them like
//    MIDI from the DIN socket.
//
//  - Device monitor (monitor.html, /api/monitor/...): shows what the
//    fenderswitch reports on its Serial output (the debug text on TX): its
//    display, channel LEDs and log. A real MIDI cable has no return path; this
//    only exists because USB carries both directions.
//
// index.html shows both side by side.
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

// ===========================================================================
// Serial link: the "cable" between footboard and device.

let port = null;
// True once the port is open and the Nano has booted.
let linkReady = false;

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
      .on('data', (line) => monitorLine(line.replace(/\r$/, '')));
    setTimeout(() => {
      if (port === p) {
        console.log('serial: ready');
        linkReady = true;
        setDevice({ connected: true });
      }
    }, BOOT_WAIT_MS);
  });
  p.on('close', () => {
    console.error(`serial: ${SERIAL_PORT} closed, retrying in ${RETRY_MS / 1000} s`);
    port = null;
    linkReady = false;
    setDevice({ connected: false });
    setTimeout(openPort, RETRY_MS);
  });
  p.on('error', (err) => console.error(`serial: ${err.message}`));
}

// ===========================================================================
// Footboard: MIDI out. Knows only program numbers.

// Sends Program Change for program 1-128 on MIDI_CHANNEL.
function sendProgramChange(program) {
  const bytes = [0xC0 | (MIDI_CHANNEL - 1), program - 1];
  port.write(Buffer.from(bytes));
  // The monitor shows the bytes on the wire, like a MIDI analyzer would.
  monitorBroadcast('log', {
    dir: 'midi',
    text: `${bytes.map((b) => b.toString(16).padStart(2, '0')).join(' ')}  program change ch=${MIDI_CHANNEL} pc=${program}`,
  });
}

// ===========================================================================
// Device monitor: what the fenderswitch reports on its Serial output.

const device = {
  connected: false,   // serial link up and the Nano done booting
  program: null,      // 1-128
  ampChannel: null,   // "Clean", "Drive" or "More Drive"
  modified: false,    // preset changed with a channel button, not stored
};

const monitorClients = new Set();

function monitorBroadcast(event, data) {
  const message = `event: ${event}\ndata: ${JSON.stringify(data)}\n\n`;
  for (const res of monitorClients) {
    res.write(message);
  }
}

function setDevice(changes) {
  Object.assign(device, changes);
  monitorBroadcast('device', device);
}

// Parses one line of the firmware's Serial output.
//   program=12 channel=Drive (modified)
//   stored program=12 channel=Drive
// This is the firmware's human-readable debug text, not a fixed interface:
// if its wording changes, the display stops updating (the log still works).
function monitorLine(line) {
  monitorBroadcast('log', { dir: 'serial', text: line });
  const match = /^(?:stored )?program=(\d+) channel=(Clean|Drive|More Drive)( \(modified\))?$/.exec(line);
  if (match) {
    setDevice({
      program: Number(match[1]),
      ampChannel: match[2],
      modified: Boolean(match[3]),
    });
  }
}

// ===========================================================================
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

// Footboard API.
//   GET  /api/midi/config          -> {midiChannel}
//   POST /api/midi/program-change  {program: 1-128}
async function handleMidi(req, res, pathname) {
  if (req.method === 'GET' && pathname === '/api/midi/config') {
    return sendJson(res, 200, { midiChannel: MIDI_CHANNEL });
  }
  if (req.method === 'POST' && pathname === '/api/midi/program-change') {
    let program;
    try {
      ({ program } = await readJson(req));
    } catch {
      return sendJson(res, 400, { error: 'expected JSON {"program": 1-128}' });
    }
    if (!Number.isInteger(program) || program < 1 || program > PROGRAM_COUNT) {
      return sendJson(res, 400, { error: 'program must be an integer 1-128' });
    }
    if (!linkReady) {
      return sendJson(res, 503, { error: 'MIDI out not connected' });
    }
    sendProgramChange(program);
    return sendJson(res, 200, { ok: true, program });
  }
  res.writeHead(404).end();
}

// Monitor API.
//   GET /api/monitor/events  Server-Sent Events: config, device, log
function handleMonitor(req, res, pathname) {
  if (req.method === 'GET' && pathname === '/api/monitor/events') {
    res.writeHead(200, {
      'Content-Type': 'text/event-stream',
      'Cache-Control': 'no-cache',
      Connection: 'keep-alive',
    });
    res.write(`event: config\ndata: ${JSON.stringify({ serialPort: SERIAL_PORT })}\n\n`);
    res.write(`event: device\ndata: ${JSON.stringify(device)}\n\n`);
    monitorClients.add(res);
    req.on('close', () => monitorClients.delete(res));
    return;
  }
  res.writeHead(404).end();
}

async function serveStatic(req, res, pathname) {
  const name = pathname === '/' ? 'index.html' : pathname.slice(1);
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
}

async function handleRequest(req, res) {
  const { pathname } = new URL(req.url, 'http://localhost');
  if (pathname.startsWith('/api/midi/')) return handleMidi(req, res, pathname);
  if (pathname.startsWith('/api/monitor/')) return handleMonitor(req, res, pathname);
  if (req.method === 'GET') return serveStatic(req, res, pathname);
  res.writeHead(405).end();
}

// Keeps idle SSE connections alive through proxies and sleeping phones.
setInterval(() => {
  for (const res of monitorClients) res.write(': ping\n\n');
}, 15000);

http.createServer((req, res) => {
  handleRequest(req, res).catch((err) => {
    console.error(err);
    if (!res.headersSent) res.writeHead(500);
    res.end();
  });
}).listen(HTTP_PORT, HOST, () => {
  console.log(`footboard + monitor: http://localhost:${HTTP_PORT}/ (MIDI channel ${MIDI_CHANNEL})`);
  console.log(`  footboard only:    http://localhost:${HTTP_PORT}/footboard.html`);
  console.log(`  monitor only:      http://localhost:${HTTP_PORT}/monitor.html`);
});

openPort();
