from flask import Flask, request
import requests

app = Flask(__name__)

# dict of board names to their addresses
boards = {}
# how long to wait in seconds before assuming a previosuly connected board has been turned off
TIMEOUT = 3

@app.get("/boards")
def all_board_names():
    active_boards = {}

    for name, address in boards.items():
        try:
            status = requests.get(address, timeout=TIMEOUT).status_code
            active_boards[name] = address
        except requests.exceptions.ConnectTimeout:
            continue

    boards = active_boards

    return active_boards, 200


@app.post("/boards/<board_name>")
def add_board(board_name):
    address = request.data.decode("utf-8")
    address_response_code = requests.get(address).status_code

    if str(address_response_code).startswith("2"):
        boards[board_name] = address
        return '', 201
    else:
        return "Given address gave a non-2XX response", address_response_code


def on_board_name_exists(board_name, on_success):
    if board_name in boards:
        return on_success()
    else:
        return f"Board name {board_name} not found", 404


@app.get("/boards/<board_name>")
def get_board_state(board_name):
    def on_success():
        address = boards[board_name]
        resp = requests.get(address)
        return resp.content, resp.status_code, resp.headers.items()

    return on_board_name_exists(board_name, on_success)


@app.delete("/boards/<board_name>")
def remove_board(board_name):
    def on_success():
        del boards[board_name]
        return '', 200

    return on_board_name_exists(board_name, on_success)


@app.patch("/boards/<board_name>/enabled/<enable>")
def set_board_is_enabled(board_name, enable):
    def on_success():
        address = boards[board_name]
        enabling_address = f"{address}/?enabled={enable}"
        resp = requests.patch(enabling_address)
        return resp.content, resp.status_code, resp.headers.items()

    return on_board_name_exists(board_name, on_success)
