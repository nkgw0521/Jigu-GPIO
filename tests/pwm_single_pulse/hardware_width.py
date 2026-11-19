"""Test-only capture firmware: first/last edge timestamps over the existing loopback.
The application must be restored to production firmware by the caller afterward.
"""
import argparse,json,re,socket,subprocess,time
from pathlib import Path
import serial
p=argparse.ArgumentParser();p.add_argument('--port',required=True);p.add_argument('--tcl',type=int,required=True);p.add_argument('--elf',required=True);p.add_argument('--nm',required=True);p.add_argument('--mhz',type=int,required=True);p.add_argument('--output',required=True);p.add_argument('--polarity',type=int,choices=[0,1]);p.add_argument('--tolerance',type=int,default=2);a=p.parse_args()
symbols={}
for line in subprocess.check_output([a.nm,a.elf],text=True).splitlines():
 fields=line.split()
 if len(fields)==3 and fields[2].startswith('probe_'):symbols[fields[2]]=int(fields[0],16)
rows=[]
with socket.create_connection(('127.0.0.1',a.tcl),timeout=3) as sock,serial.Serial(a.port,115200,timeout=.03,write_timeout=2) as s:
 def rpc(c):
  sock.sendall(c.encode()+b'\x1a');b=b''
  while not b.endswith(b'\x1a'):b+=sock.recv(8192)
  return b[:-1].decode().strip()
 def read(name):return int(rpc('read_memory %d 32 1'%symbols[name]).split()[0],0)
 def cmd(c):
  s.reset_input_buffer();s.write((c+'\n').encode());s.flush();b=b'';deadline=time.monotonic()+2
  while time.monotonic()<deadline:
   b+=s.read(512);ls=[x.strip() for x in b.decode(errors='replace').splitlines() if x.strip() and x.strip()!=c]
   if '?' not in c or ls:return ls[-1] if ls else ''
  raise RuntimeError((c,b))
 try:
  for pol in ([a.polarity] if a.polarity is not None else [0,1]):
   for hz,widths,repeats in [(1000,[1,2,5,10,100,500],3),(10000,[1,10],2)]:
    for width in widths:
     for n,target in [(1,1),(3,1),(3,3)]:
      for repeat in range(repeats):
       cmd('PWM:STart 0');cmd('PWM:Width 1');cmd('PWM:Frequency '+str(hz));cmd('PWM:Width '+str(width));cmd('PWM:POLarity '+str(pol));cmd('PWM:Numbers '+str(n));cmd('PWM:SYNC 0')
       rpc('write_memory %d 32 {%d}'%(symbols['probe_target'],target))
       cmd('PWM:STart 1');deadline=time.monotonic()+2
       while cmd('PWM:STart?')!='0':
        if time.monotonic()>deadline:raise RuntimeError('timeout')
       ready=read('probe_ready');start=read('probe_start');end=read('probe_end');status=read('probe_status');index=read('probe_index')
       ticks=(end-start)&65535;expected=width*a.mhz;error=cmd('SYSTem:ERRor?')
       row=dict(Hz=hz,RequestedUs=width,Polarity=pol,Pulses=n,MeasuredPulse=target,Repeat=repeat+1,StartTicks=start,EndTicks=end,WidthTicks=ticks,ExpectedTicks=expected,MeasuredUs=ticks/a.mhz,CaptureStatus=status,CaptureReady=ready,CapturedPulse=index,Error=error,Pass=ready==1 and index==target and abs(ticks-expected)<=a.tolerance and bool(status&2) and not(status&0x600) and error.startswith('0,'))
       rows.append(row);Path(a.output).write_text(json.dumps(dict(Port=a.port,Method='external loopback, test-only hardware IC1/IC2 timestamps, no breakpoint pauses; calibrated absolute time not verified',ClockMHz=a.mhz,ToleranceTicks=a.tolerance,Results=rows),indent=2)+'\n')
       if not row['Pass']:raise AssertionError(row)
    print(a.port,'widths passed',len(rows),'polarity',pol,'Hz',hz,flush=True)
 finally:cmd('PWM:STart 0')
print('PASS',a.port,'width captures',len(rows))
