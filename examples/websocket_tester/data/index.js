class graph{
	decorate(){
		var incr=this.maxY/this.ticks;
		var c=this.ctx;
	
		c.fillStyle="#000";
		c.font = '8px sans-serif';	
		c.beginPath();
		for(var i=0;i<this.ticks-1;i++){
			var y=incr*(i+1);
			var plotY=this.norm(y);
			c.moveTo(0,plotY);
			c.lineTo(10,plotY);
			c.fillText(Math.round(y),12,plotY);	
		}
		c.stroke();
	}	
	
	drawFrame(){
		var c=this.ctx;
		c.fillStyle="#fff";
		c.fillRect(0,0,this.width,this.height);
		c.strokeStyle="#000";
		c.strokeRect(0,0,this.width,this.height);		
		this.decorate();
	}

	constructor(ele,maxY,base,ticks,dp){
		this.ele=ele;
		this.maxY=maxY;
		this.base=base;
		this.ticks=ticks;
		this.dp=dp;		
		var g=document.getElementById(ele);
		this.ctx=g.getContext("2d");
		this.height=g.height;
		this.width=g.width;
		this.lastv=0;
		this.nPoints=this.width/base;
		this.points=[];
		for(var i=0;i<this.nPoints;i++)	this.points[i]=0;		
	}
	
	norm(v){
		var h=this.height;
		return h-((v/this.maxY)*h);
	}

	plot(v){
		var c=this.ctx;
		c.strokeStyle="#00f";		
		
		this.points.shift();
		this.points.push(v);
		
		this.drawFrame();
		var x=0;
		var y=0;	
		this.sigma=0;
		var slide=parseInt(this.base);
		var c=this.ctx;
		c.strokeStyle="#00f";		
		
		c.beginPath();		
		for(var i=0;i<this.nPoints;i++){
			y=this.points[i];
			this.sigma+=parseInt(y);
			c.lineTo(x,this.norm(y));
			x+=slide;
		}
		c.stroke();
		
		var avg=this.sigma/this.nPoints;
		ibi("R1cma",avg.toFixed(0));

		c.beginPath();		
			c.strokeStyle="#f00";
			c.setLineDash([5, 5]);
			c.moveTo(0,this.norm(avg));
			c.lineTo(this.width,this.norm(avg));
		c.stroke();
		
		c.setLineDash([]);
		c.fillStyle="#f00";
		c.font = '8px sans-serif';
		c.fillText(avg.toFixed(this.dp),this.width / 2 ,this.norm(avg) - 3);
	}
}

function dgsDiv(c,t,i,hang){
	var dgs=document.createElement("div");
	dgs.className=c;
	dgs.innerHTML=t;
	hang.appendChild(dgs);
	
	var dgs2=document.createElement("div");
	dgs2.className="right";
	var dgs3=document.createElement("div");
	dgs3.id=i;
	dgs3.className="std";
	dgs3.innerHTML="0";
	dgs2.appendChild(dgs3);
	hang.appendChild(dgs2);  
}

function doGraph(m){
	var g=document.getElementById("G");
	var sth=document.createElement("div");
	sth.className="sth";
	dgsDiv("stm",m+":",m,sth);
	dgsDiv("stt","cma:",m+"cma",sth);
	g.appendChild(sth);
  
	var can=document.createElement("div");
	can.className="can";
	var vas=document.createElement("canvas");
	vas.id="gc"+m;
	vas.width="360";
	vas.height="100";
	can.appendChild(vas);
	g.appendChild(can);  
}

function ibi(e,v){ document.getElementById(e).innerHTML=v; }

function doPlot(data){
	var pd=data;
	var base="R1";
	grf.plot(pd.c);
	ibi(base,pd.c);
}

let ws
let grf

function doMsg(m){	document.getElementById("msg").innerHTML=m; }

function sendUser(){
	ws.send(document.getElementById("user").value)
	grf.strike(42)
}
document.addEventListener("DOMContentLoaded", function() {
	doGraph("R1");
	grf=new graph("gcR1",1000,2,10,0); //ele,maxY,base,ticks,dp

    doMsg("Opening connection...")
	ws = new WebSocket("ws://"+document.location.host+"/ws");

	ws.onerror = function(e) {
		console.warn(e);
		doMsg("Error "+e)
	};
	ws.onopen = function() { 
		ws.send("Hello there!");
		doMsg("Connection open")
	};
	ws.onmessage = function (evt) {
		let parts=evt.data.split(",");
		let ele=document.getElementById(parts[0]);
		if(parts[0]=="rnd1") doPlot({'c': parseInt(parts[1]), 'n':1,'x':2,'a':3});
		else {
			if(ele) ele.innerHTML=parts[1];
			else doMsg(parts[0])	
		}
	};
	ws.onclose = function() { doMsg("Connection closed") }
})