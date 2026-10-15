"""
Optional AI generation bridge for Wonder Melody Generator.
Requires Python 3.10+ and: pip install flask openai
Set OPENAI_API_KEY in your environment before starting.
Run: python server.py
The plugin sends prompts to localhost only; this server calls the OpenAI API.
"""
import json
import os
from flask import Flask, request, jsonify
from openai import OpenAI

app = Flask(__name__)
client = OpenAI()  # reads OPENAI_API_KEY from environment

SYSTEM = """You are an expert music composition assistant. Return ONLY valid JSON with this schema:
{
 "melody":[32 integers; MIDI pitch 48-95 or -1 for rest],
 "chords":[8 arrays of 3 MIDI pitches],
 "bass":[8 MIDI pitches],
 "drums":[16 booleans]
}
Create coherent, playable original patterns. Respect the requested key and tempo. Use a minor-key scale unless asked otherwise. Chords should be triads with sensible voice leading; bass should support each chord; drum steps should form a groove. Never include markdown fences or prose."""

@app.post("/generate")
def generate():
    data = request.get_json(force=True)
    prompt = str(data.get("prompt", "Create a modern beat idea"))[:2000]
    model = str(data.get("model", "gpt-4.1-mini"))[:100]
    key = int(data.get("key", 0))
    tempo = int(data.get("tempo", 140))
    try:
        response = client.chat.completions.create(
            model=model,
            response_format={"type": "json_object"},
            messages=[
                {"role": "system", "content": SYSTEM},
                {"role": "user", "content": f"Prompt: {prompt}\nRoot pitch class: {key}\nTempo: {tempo} BPM."}
            ],
            temperature=0.9,
        )
        result = json.loads(response.choices[0].message.content)
        # Validate and normalize arrays before passing data into the plugin.
        melody = result.get("melody", [])
        chords = result.get("chords", [])
        bass = result.get("bass", [])
        drums = result.get("drums", [])
        if len(melody) != 32 or len(chords) != 8 or len(bass) != 8 or len(drums) != 16:
            return jsonify({"error": "Model returned incorrect pattern dimensions"}), 422
        result["melody"] = [max(-1, min(127, int(x))) for x in melody]
        result["chords"] = [[max(0, min(127, int(n))) for n in triad[:3]] for triad in chords]
        result["bass"] = [max(0, min(127, int(x))) for x in bass]
        result["drums"] = [bool(x) for x in drums]
        return jsonify(result)
    except Exception as exc:
        return jsonify({"error": str(exc)}), 500

if __name__ == "__main__":
    app.run(host="127.0.0.1", port=8765, debug=False)
