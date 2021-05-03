'use strict';
; (function (document, window, index) {
    let form = document.querySelector('.box');

    let label = form.querySelector('label');
    let successMsg = form.querySelector('.box__success span');
    let errorMsg = form.querySelector('.box__error span');
    let restart = form.querySelector('.box__restart');
    let droppedFiles = false;
    let triggerFormSubmit = function () {
        let event = document.createEvent('HTMLEvents');
        event.initEvent('submit', true, false);
        form.dispatchEvent(event);
    };

    ['drag', 'dragstart', 'dragend', 'dragover', 'dragenter', 'dragleave', 'drop'].forEach(function (event) {
        form.addEventListener(event, function (e) {
            // preventing the unwanted behaviours
            e.preventDefault();
            e.stopPropagation();
        });
    });

    ['dragover', 'dragenter'].forEach(function (event) {
        form.addEventListener(event, function () {
            form.classList.add('is-dragover');
        });
    });

    ['dragleave', 'dragend', 'drop'].forEach(function (event) {
        form.addEventListener(event, function () {
            form.classList.remove('is-dragover');
        });
    });

    form.addEventListener('drop', function (e) {
        droppedFiles = e.dataTransfer.files; // the files that were dropped
        triggerFormSubmit();
    });

    // if the form was submitted
    form.addEventListener('submit', function (e) {
        // preventing the duplicate submissions if the current one is in progress
        if (form.classList.contains('is-uploading')) return false;

        form.classList.add('is-uploading');
        form.classList.remove('is-error');

        e.preventDefault();

        // gathering the form data
        //////////////////////////
        var ajaxData = new FormData();

        if (droppedFiles) {
            Array.prototype.forEach.call(droppedFiles, function (file) {
                ajaxData.append(file.name, file);
            });
        }

        // ajax request
        var ajax = new XMLHttpRequest();
        ajax.open(form.getAttribute('method'), form.getAttribute('action'), true);

        ajax.onload = function () {
            form.classList.remove('is-uploading');
            if (ajax.status >= 200 && ajax.status < 400) {
                console.log(ajax.responseText);
                var data = JSON.parse(ajax.responseText);
                form.classList.add(data.success == true ? 'is-success' : 'is-error');
                if (!data.success) errorMsg.textContent = data.error;
            }
            else alert('Error. Please, contact the webmaster!');
        };

        ajax.onerror = function () {
            form.classList.remove('is-uploading');
            alert('Error. Please, try again!');
        };

        for (var key of ajaxData.keys()) {
            console.log("key " + key);
        }
        for (var val of ajaxData.values()) {
            console.log("val " + val);
        }

        ajax.send(ajaxData);
        successMsg.textContent = (droppedFiles.length > 1 ? droppedFiles.length + ' files' : droppedFiles[0].name) + ' uploaded!';
    });


    restart.addEventListener('click', function (e) {
        e.preventDefault();
        form.classList.remove('is-error', 'is-success');
    });

    // init
    function init() {
        console.log("js init");
        
        // let listReq = new XMLHttpRequest();
        // listReq.open("get", "/list", true);
        // listReq.onreadystatechange = function() {
        //    if (listReq.readyState == XMLHttpRequest.DONE) {
        //        console.log("/list\n" + req.responseText);

                let fileList = [
                    "test.gif", 
                    "test1.gif", 
                    "test2.gif",
                    "test3.gif"
                 ];
                generatePlaylist(fileList);
        //    }
        // }
        // listReq.send(null);

        // semantic ui, enable tab
        $('.tabular.menu .item').tab();
    }

    function generatePlaylist(fileList) {
        
        var playlist = document.getElementById("playlist");
        var filemanager = document.getElementById("filemanager");

        for(let i=0; i<fileList.length; i++){
            
            //let filePath = "./gifs/test.gif";
            let filename = fileList[i];
            
            let item = document.createElement("div");
            item.classList.add("item", filename);

            let img = document.createElement("img");
            img.classList.add("ui", "left", "floated", "mini", "image");
            img.src = "./gifs/" + filename;
            
            let content = document.createElement("div");
            content.classList.add("content");            
            content.innerText = filename;

            let playBtn = document.createElement("div");
            playBtn.classList.add("ui", "right", "floated", "blue", "button");
            playBtn.innerText = "play";
            playBtn.onclick = function(){
                console.log("Play button, clicked!!");
                playFile(filename);
            };

            let deleteBtn = document.createElement("div");
            deleteBtn.classList.add("ui", "right", "floated", "red", "button");
            deleteBtn.innerText = "delete";
            deleteBtn.onclick = function(){
                console.log("delete button, clicked!!");
                deleteFile(filename);
            };
            
            item.appendChild(img);
            item.appendChild(content);
            
            let item2 = item.cloneNode( true );
            item.appendChild(playBtn);
            item2.appendChild(deleteBtn);
            
            playlist.appendChild(item);
            filemanager.appendChild(item2);

        }
    }

    function deleteFile( filename ){
        let els = document.getElementsByClassName(filename);
        console.log(els.length);
        
        while(els.length>0){
            let parent = els[0].parentNode;
            parent.removeChild(els[0]);
        }
        
        // var req = new XMLHttpRequest();
        // let url = "/delete?" + filename;
        // req.open("delete", url, true);
        // req.onreadystatechange = function() {
        //    if (req.readyState == XMLHttpRequest.DONE) {
        //        console.log("/delete, done");
        //        this.parentNode.removeChild(this);
        //    }
        // }

        // req.send(null);
    }

    function playFile( filename ){
        var req = new XMLHttpRequest();
        let url = "/play?" + filename;
        req.open("post", url, true);
        req.send(null);
    }

    init();

}(document, window, 0));
