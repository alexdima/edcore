/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

var index = require('./index');

var EdBuffer = index.EdBuffer;

var buff = new EdBuffer();

console.log(`line count: ${buff.GetLineCount()}`);
