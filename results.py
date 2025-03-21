import matplotlib.pyplot as plt
import pandas as pd

# CSV 파일에는 시간(time), throughput(Mbps), delay(ms), loss(%) 컬럼이 포함되어야 함
data = pd.read_csv('results.csv')

# 1️⃣ Throughput 그래프
plt.figure()
plt.plot(data['time'], data['throughput'], marker='o')
plt.title('Throughput Over Time (GEO Simulation)')
plt.xlabel('Time (s)')
plt.ylabel('Throughput (Mbps)')
plt.grid()
plt.savefig('throughput.png')

# 2️⃣ Delay 그래프
plt.figure()
plt.plot(data['time'], data['delay'], marker='x', color='orange')
plt.title('Delay Over Time (GEO Simulation)')
plt.xlabel('Time (s)')
plt.ylabel('Delay (ms)')
plt.grid()
plt.savefig('delay.png')

# 3️⃣ Packet Loss 그래프
plt.figure()
plt.plot(data['time'], data['loss'], marker='s', color='red')
plt.title('Packet Loss Over Time (GEO Simulation)')
plt.xlabel('Time (s)')
plt.ylabel('Loss (%)')
plt.grid()
plt.savefig('loss.png')

print(\"그래프 3종 저장 완료: throughput.png / delay.png / loss.png\")
