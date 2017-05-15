/*---------------------------------------------------------------------------------------------
 *  Copyright (c) Microsoft Corporation. All rights reserved.
 *  Licensed under the MIT License. See License.txt in the project root for license information.
 *--------------------------------------------------------------------------------------------*/

var binding = require('./build/Release/edcore');

console.log('OK');

exports.EdBuffer = binding.EdBuffer;
