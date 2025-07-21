# --- Using dareplane_utils ------------
from dareplane_utils.stream_watcher.lsl_stream_watcher import StreamWatcher

sw = StreamWatcher(name="AODataStream")
sw.connect_to_stream()
sw.update()
sw.unfold_buffer()


# --- Using mne_lsl ------------
from mne_lsl.stream import StreamLSL as Stream

s = Stream(bufsize=2, name="AODataStream")
s.connect()
s.get_data()
