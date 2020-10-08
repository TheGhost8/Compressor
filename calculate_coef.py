import pandas as pd
data = pd.read_csv('results.csv')
raw_data = 0
compressed_data = 0
for i in range(8):
	raw_data += data['original_size'][i]
	compressed_data += data['compressed_size'][i]
print(data)
print("%.3f" % (compressed_data/raw_data))