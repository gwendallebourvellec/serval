/* -*- Mode: Java; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 *  Licensed to the Apache Software Foundation (ASF) under one or more
 *  contributor license agreements.  See the NOTICE file distributed with
 *  this work for additional information regarding copyright ownership.
 *  The ASF licenses this file to You under the Apache License, Version 2.0
 *  (the "License"); you may not use this file except in compliance with
 *  the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
package serval.net;

import java.io.IOException;
import java.io.InputStream;
import java.net.Socket;


/**
 * The SocketInputStream supports the streamed reading of bytes from a socket.
 * Multiple streams may be opened on a socket, so care should be taken to manage
 * opened streams and coordinate read operations between threads.
 */
class ServalSocketInputStream extends InputStream {

    private final ServalPlainSocketImpl socket;

    /**
     * Constructs a ServalSocketInputStream for the <code>socket</code>. Read
     * operations are forwarded to the <code>socket</code>.
     * 
     * @param socket the socket to be read
     * @see Socket
     */
    public ServalSocketInputStream(ServalSocketImpl socket) {
        super();
        this.socket = (ServalPlainSocketImpl) socket;
    }

    @Override
    public int available() throws IOException {
        return socket.available();
    }

    @Override
    public void close() throws IOException {
        socket.close();
    }

    @Override
    public int read() throws IOException {
        byte[] buffer = new byte[1];
        int result = socket.read(buffer, 0, 1);
        return (-1 == result) ? result : buffer[0] & 0xFF;
    }

    @Override
    public int read(byte[] buffer) throws IOException {
        return read(buffer, 0, buffer.length);
    }

    @Override
    public int read(byte[] buffer, int offset, int count) throws IOException {
        if (null == buffer) {
            throw new IOException("IO exception");
        }

        if (0 == count) {
            return 0;
        }

        if (0 > offset || offset >= buffer.length) {
            // K002e=Offset out of bounds \: {0}
            throw new ArrayIndexOutOfBoundsException("Out of bounds. Offset=" + offset);
        }
        if (0 > count || offset + count > buffer.length) {
            throw new ArrayIndexOutOfBoundsException("Out of bounds");
        }

        return socket.read(buffer, offset, count);
    }

    @Override
    public long skip(long n) throws IOException {
        return (0 == n) ? 0 : super.skip(n);
    }
}
