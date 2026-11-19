import json,time
from pathlib import Path
import serial
r=Path(__file__).resolve().parents[2]
for port,folder,width in [('COM6','nucleo_l053r8',1),('COM16','f303_counter32',100)]:
 rows=[]
 with serial.Serial(port,115200,timeout=.04,write_timeout=2) as s:
  def cmd(c):
   s.reset_input_buffer();s.write((c+'\n').encode());b=b'';deadline=time.monotonic()+2
   while time.monotonic()<deadline:
    b+=s.read(256);ls=[x.strip() for x in b.decode(errors='replace').splitlines() if x.strip() and x.strip()!=c]
    if '?' not in c or ls:return ls[-1] if ls else ''
   raise RuntimeError((c,b))
  try:
   for pol in [0,1]:
    for c in ['PWM:STart 0','PWM:Width 1','PWM:Frequency 1000','PWM:Width 10','PWM:POLarity '+str(pol),'PWM:SYNC 0']:cmd(c)
    for n in [1,2,10,1]:
     cmd('PWM:Numbers '+str(n));cmd('PWM:Counter 0');cmd('PWM:STart 1');time.sleep(.03)
     count=int(cmd('PWM:Counter?'));running=cmd('PWM:STart?');error=cmd('SYSTem:ERRor?')
     row=dict(Polarity=pol,Pulses=n,Count=count,Running=running,Error=error,Pass=count==n and running=='0' and error.startswith('0,'));rows.append(row);assert row['Pass'],row
    cmd('PWM:Numbers 0');cmd('PWM:Counter 0');cmd('PWM:STart 1');time.sleep(.05);a=int(cmd('PWM:Counter?'));time.sleep(.05);b=int(cmd('PWM:Counter?'));cmd('PWM:STart 0');end=int(cmd('PWM:Counter?'));time.sleep(.1);stable=int(cmd('PWM:Counter?'))
    row=dict(Polarity=pol,Mode='continuous then explicit stop',First=a,Second=b,Stopped=end,Stable=stable,Pass=0<a<b<=end and end==stable);rows.append(row);assert row['Pass'],row
  finally:
   for c in ['PWM:STart 0','PWM:Frequency 50','PWM:Width '+str(width),'PWM:POLarity 1','PWM:Numbers 0','PWM:SYNC 0','PWM:Counter 0']:cmd(c)
  final={k:cmd('PWM:'+k+'?') for k in ['STart','Frequency','Width','POLarity','Numbers','SYNC','Counter']};assert final==dict(STart='0',Frequency='50',Width=str(width),POLarity='1',Numbers='0',SYNC='0',Counter='0'),final
  (r/'tests'/folder/'idle-handoff-final-check.json').write_text(json.dumps(dict(Port=port,Firmware='production release restored and verified after capture tests',Results=rows,FinalSettings=final),indent=2)+'\n')
 print('PASS final production firmware',port,len(rows),final,flush=True)
