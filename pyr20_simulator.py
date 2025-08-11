import serial
import time
import struct

PORT = 'COM13'       # Porta do conversor USB-RS485
BAUDRATE = 9600

irradiance = 0  # começa em 0

def calc_crc(data: bytes) -> bytes:
    crc = 0xFFFF
    for pos in data:
        crc ^= pos
        for _ in range(8):
            if crc & 1:
                crc >>= 1
                crc ^= 0xA001
            else:
                crc >>= 1
    return struct.pack('<H', crc)  # Little endian

def build_response(irradiance: int) -> bytes:
    resp = bytearray()
    resp.append(0x01)         # Slave ID
    resp.append(0x04)         # Function code
    resp.append(0x02)         # Byte count
    resp += struct.pack('>H', irradiance)  # Big endian
    resp += calc_crc(resp)
    return resp

def run_slave():
    global irradiance
    last_update = time.time()

    with serial.Serial(PORT, BAUDRATE, bytesize=8, parity='N', stopbits=1, timeout=0.1) as ser:
        print(f"Modbus RTU slave iniciado em {PORT} @ {BAUDRATE} bps")
        while True:
            # Incrementa a cada 1 segundo
            if time.time() - last_update >= 1.0:
                irradiance = (irradiance + 1) % 2001  # de 0 a 2000
                last_update = time.time()

            req = ser.read(8)
            if len(req) == 8:
                slave_id, func, addr_hi, addr_lo, q_hi, q_lo, crc_lo, crc_hi = req
                if slave_id == 1 and func == 0x04 and addr_hi == 0x00 and addr_lo == 0x00:
                    print(f"[{time.strftime('%H:%M:%S')}] Req: {req.hex(' ').upper()} | Irr: {irradiance}")
                    response = build_response(irradiance)
                    ser.write(response)
                    print(f"                 Resp: {response.hex(' ').upper()}")
            time.sleep(0.01)

if __name__ == '__main__':
    run_slave()
