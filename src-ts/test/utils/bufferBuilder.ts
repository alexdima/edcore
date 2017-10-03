/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

import * as path from 'path';
import * as fs from 'fs';
import { EdBuffer, EdBufferBuilder } from '../../../index';

const FIXTURES_FOLDER = path.join(__dirname, '../../../test/fixtures');

export function buildBufferFromFixture(fileName: string, chunkSize: number = 1 << 16): EdBuffer {
    const fileContentsStr = readFixture(fileName);

    const builder = new EdBufferBuilder();
    let offset = 0;
    while (offset < fileContentsStr.length) {
        const toOffset = Math.min(offset + chunkSize, fileContentsStr.length);
        builder.AcceptChunk(fileContentsStr.substring(offset, toOffset));
        offset = toOffset;
    }
    builder.Finish();
    return builder.Build();
}

const FIXTURE_CACHE: { [fileName:string]: string; } = {};

export function readFixture(fileName: string): string {
    if (!FIXTURE_CACHE.hasOwnProperty(fileName)) {
        const filePath = path.join(FIXTURES_FOLDER, fileName);
        const fileContents = fs.readFileSync(filePath);
        FIXTURE_CACHE[fileName] = fileContents.toString();
    }
    return FIXTURE_CACHE[fileName];
}
