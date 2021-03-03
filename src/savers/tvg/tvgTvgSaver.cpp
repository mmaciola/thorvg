/*
 * Copyright (c) 2020 Samsung Electronics Co., Ltd. All rights reserved.

 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:

 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <string.h>  //MGS ??
#include "tvgTvgSaver.h"

/************************************************************************/
/* Internal Class Implementation                                        */
/************************************************************************/

bool header(char** pointer)
{
cout << __FILE__ << " " << __func__ << endl;
    // MGS - hardcoded for now
    const char *tvg = "TVG";
    memcpy(*pointer, tvg, 3);
    *pointer += 3;
    const char *version = "000";
    memcpy(*pointer, version, 3);
    *pointer += 3;
    // metadata
    *pointer += 2;

    return true;
}

/*
bool saveScene()
{
cout << __FILE__ << " " << __func__ << endl;
    // MGS TODO 
    return true;
}
*/

/************************************************************************/
/* External Class Implementation                                        */
/************************************************************************/

//TvgSaver::TvgSaver()
TvgSaver::TvgSaver(Scene* scene) : root(scene)  //MGS2
{
cout << __FILE__ << " " << __func__ << endl;
}

TvgSaver::~TvgSaver()
{
cout << __FILE__ << " " << __func__ << endl;
    close();
}

void TvgSaver::run(unsigned tid)
{
cout << __FILE__ << " " << __func__ << endl;
//MGS5
//    Saver::nic();

    root->serialize();  //MGS temp bo nie moge tego obejsc

// MGS2
/* 
  ale tak sie nie da, bo nie mam dostepu do pImpla bo jest protected
  dalam scenie friend saver, no ale to saver ma dostep a nie tvgsaver

  chce tutaj wywolac fun ze sceny z pImpla serialize
*/
    //scene->pImpl->serialize(&pointer); 
//    if (!saveScene()) return; 
};

void TvgSaver::resizeBuffer()
{
    reserved *= 2;
    buffer = static_cast<char*>(realloc(buffer, reserved));
// MGS8 - obsluga bledy realloca dodac
}


bool TvgSaver::open(const string& path)
{
cout << __FILE__ << " " << __func__ << endl;
    // MGS - open the file here? 
    outFile.open(path, ios::out | ios::trunc | ios::binary);
    if (!outFile.is_open())
    {
        //LOG: Failed to open file
        return false;
    }

    reserved = 250000;//1024;
    buffer = static_cast<char*>(malloc(reserved));
    if (!buffer) {
        reserved = 0;
        // MGS - close the file or move it from here
        return false;
    }
    memset(buffer, '\0', reserved);  //MGS3
    pointer = buffer;

    return header(&pointer);
}

bool TvgSaver::write()
{
cout << __FILE__ << " " << __func__ << endl;
    if (!buffer || reserved == 0) return false;

    TaskScheduler::request(this);

    return true;
}


bool TvgSaver::close()
{
cout << __FILE__ << " " << __func__ << endl;
    this->done();

    // MGS - temp sollution ?
    outFile.write(buffer,reserved);
    //outFile.write(buffer,size);  //MGS - na razie nei zmieniam size!!! MGS8
    outFile.close();

    if (buffer) free(buffer);
    size = 0;
    reserved = 0;

    return true;
}
