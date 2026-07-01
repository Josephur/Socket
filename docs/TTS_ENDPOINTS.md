# TTS Endpoints Reference

Two 24x7 TTS backends are available on the local network. **Neither is OpenAI-compatible** — both expose their own bespoke REST APIs, not the `/v1/audio/speech` schema. Use the scripts in this repo as working examples.

## F5-TTS — `http://mouth.stack-tech.local:7860`

Docs: https://github.com/SWivid/F5-TTS

Zero-shot voice cloning model. **No built-in speaker list** — there is exactly one default voice baked into the server, and the intended usage is to always supply your own reference audio + matching transcript so the model clones that voice for the given text.

- `GET /health` — returns `{ model, device, cuda }`. See `f5tts_list_voices.py`.
- `GET /tts?text=...&speed=1.0&nfe_step=32` — uses the server's single default reference voice. No reference audio needed; convenient for a quick smoke test but not real cloning. See `f5tts_test.py`.
- `POST /tts` (JSON body) — the actual cloning path:
  - `text` (required)
  - `ref_audio` — base64-encoded WAV of the voice to clone, or a server-side path
  - `ref_text` — transcript of `ref_audio`; omit to let the server auto-transcribe via ASR (costs extra VRAM)
  - `speed` — default `1.0`
  - `nfe_step` — diffusion steps, default `32` (higher = better quality, slower)
  - `remove_silence` — bool, default `false`
  - `seed` — optional int for reproducibility

Reference audio tips: clean/noise-free WAV, ~3-10s, `ref_text` must exactly match what's spoken.

## XTTSv2 (Coqui) — `http://mouth.stack-tech.local:5002`

Docs: https://github.com/idiap/coqui-ai-TTS

Multi-speaker model with **both** built-in voices and cloning support.

- `GET /voices` — plain text, one entry per line: `<name> <language> <gender>`. See `xtts_list_voices.py`.
- `GET /api/tts?text=...&speaker_id=...&language_id=...` — synthesize with a built-in speaker (e.g. `"Claribel Dervla"`, `en`). See `xtts_test.py`.
- Voice cloning (not exercised by the current test scripts) is done via the standard Coqui TTS server API by posting a `speaker_wav` reference instead of `speaker_id` — check the upstream docs for the exact param name on this server build, as the coqui-ai-TTS API has changed across versions.

## Summary

| | F5-TTS | XTTSv2 |
|---|---|---|
| Port | 7860 | 5002 |
| Built-in voices | 1 default only | Many (`GET /voices`) |
| Cloning | Primary use case, via `ref_audio`/`ref_text` | Supported, secondary to built-in speakers |
| OpenAI-compatible | No | No |
