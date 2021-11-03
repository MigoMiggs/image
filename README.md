# image

To Setup run:
./setup

To Build run:
./build

Run the server via using the following:
`./server --port <...> --host <...>` 

Run the client via using the following:
`./client --port <...> --host <...> --input <...> --output <...> --rotate <...> --mean`

Examples:

./server --host 0.0.0.0 --port 50051
./client --host localhost --port 50051 --rotate TWO_SEVENTY_DEG --mean true --input ./test_neura.jpeg --output ./saved.jpeg






