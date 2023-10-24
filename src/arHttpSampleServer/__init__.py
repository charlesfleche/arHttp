import pathlib

from fastapi import FastAPI, HTTPException
from fastapi.responses import PlainTextResponse

_layersdir = pathlib.Path(__file__).parent

_ID_LAYERPATH_MAP = {
    "id0x01": str(_layersdir / "layer1.usda"),
    "id0x02": str(_layersdir / "layer2.usda"),
}

app = FastAPI()

@app.get("/{asset_path}", response_class=PlainTextResponse)
async def root(asset_path):
    if resolved_path := _ID_LAYERPATH_MAP.get(asset_path):
        return resolved_path
    raise HTTPException(status_code=404)
