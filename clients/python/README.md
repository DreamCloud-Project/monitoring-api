# RheonMedia Monitoring Demo

## Summary
The provided Python package details the basic usage of the current monitoring
API to send metrics collected to DreamClouds' monitoring database. The agent
demo registers a new workflow named 'rm_stream', and then sends some data to
the monitoring database for four tasks 'running' on different hardware. For
general information about the montoring API, please refer to the specification
found at `work/WP3/Monitoring_API`.


## Project Structure
The API is implemented in the file `mfapi.py`. For prototyping, the demo is
implemented using Python. A documentation of the API methods and functions is
maintained in `doc/mfapi.html`.

    .
    |-- demo.py             (register deployment plans and send metric data)
    |-- mfapi.py            (monitoring API imported by demo.py)
    |-- README
    |-- CHANGES
    |-- LICENSE
    |-- doc
    |   |-- mfapi.html      (documentation of mfapi.py)
    |   |-- summary.rest    (brief API of /mf/summary calls)


## Prerequisites
You can either test the demo using your own Elasticsearch database
installation, or by exploiting the shared database located at
`http://mf.excess-project.eu:3030`. It is recommend to use the shared database,
because no further setup is required for testing, except the following modules:

| Module    | Version  |
| --------- | -------- |
| Python    | >= 2.7.x |

If you intend to use you local installation of Elasticsearch, you will also
need to install your own `monitoring-server`. Please execute the following
commands:

````
git clone https://github.com/DreamCloud-Project/monitoring-server.git
cd monitoring-server

./setup.sh
./start.sh
````

The script `setup.sh` will install required dependencies such as Elasticsearch
and NodeJS, whereas `start.sh` will start both services in the background.
You should then be able to access Elasticsearch via `localhost:9200`, and
our monitoring-server through `localhost:3030`.


## Demo (demo.py)
Executing the provided demo performs the following steps:

1. Set up profiling API (`new ProfilingAPI`)
2. Register workflow and retrieve a new experiment id (`new_experiment`)
3. Add information about the allocation of tasks (`add_deployment_plan`)
4. Send some metric data (`update`)

The demo includes a workflow having two tasks. Each task is allocated onto a
specific host and device (e.g. VM, FPGA1 and so on). For each task, we send
its execution time and energy consumption to the database. You are free to
adapt those metrics to your needs. Please keep in mind that values have to
be of type int or float to be properly processed. It should be noted that the
field '@timestamp' is mandatory; it has the following format:

````
YYYY-MM-DDTHH:MM:ss.ZZZ (e.g., 2015-07-31T19:13:55.134)
````

We provide information on the API methods in `docs/mfapi.html`.


### Usage

````
hopped@winterfell:rm$ python agent.py
INFO:root:[ url: http://localhost:3030/v1/dreamcloud/mf ]
INFO:root:[ workflow_id: rm_stream ]
INFO:root:[ experiment_id: AVWmZDVZHfnWt6JXwLDl ]
INFO:root:[ deployment: http://winterfell:3030/v1/dreamcloud/mf/deployments/rm_stream/task1/embedded/d779194a0412c7a5afe90e70d5800fd9113695bc90580b39b27a98892e3480b3 ]
INFO:root:[ deployment: http://winterfell:3030/v1/dreamcloud/mf/deployments/rm_stream/task2/hpc/88c8ece21a27546d7495c52fc2d08f8c9090a7ba07e7a27dcc6eb338797dce9b ]
INFO:root:[ profile: http://winterfell:3030/v1/mf/profiles/rm_stream/task1/AVWmZDVZHfnWt6JXwLDl ]
````

## Data Retrieval
The following typical queries will retrieve information previously sent to
the database (example for workflow `rm_stream`, task `task1` and experiment ID
`AVWmZDVZHfnWt6JXwLDl`):

| Example Query | Comment |
| ------------- | ------- |
| `GET /v1/dreamcloud/mf/workflows` | Retrieve information about stored workflows (e.g., `rm_stream`) |
| `GET /v1/dreamcloud/mf/workflows/rm_stream` | Get detailed information about the `rm_stream` workflow |
| `GET /v1/dreamcloud/mf/experiments?workflows=rm_stream&details` | Get detailed information for each experiment |
| `GET /v1/mf/profiles/rm_stream/task1/AVWmZDVZHfnWt6JXwLDl` | Get specific profiling information for experiment `AVWmZDVZHfnWt6JXwLDl` |
| `GET /v1/dreamcloud/mf/profiles/rm_stream/task1/AVWmZDVZHfnWt6JXwLDl` | Get profile information (i.e., actual metric data) |
| `GET /v1/dreamcloud/mf/deployments/rm_stream/task1/embedded` | Get all deployment plans used for a workflow and task on a given platform (here: embedded) |
| `GET /v1/dreamcloud/mf/summary/rm_stream/task1/embedded` | Get summary information for all experiments performed on that platform |
| `GET /v1/dreamcloud/mf/statistics/rm_stream/task1?metric=execution_time` | Get statistics for a given metric, here `execution_time` |


## Deployment Plans/Configuration
Deployment information can be retrieved via the route
`/dreamcloud/mf/deployments/<deploymentID>`. There are no specific restrictions
to the format stored; the demo followed the format used in the HPC use case.
Please adapt the data to your needs.

Example plan:

````
  {
    'estimatedTime': 217,
    'node': {
      'id': 'embedded_device',
      'cpus': [
        {
          'id': 'cpu0',
          'cores': [
            {
              'id': 'core0',
              'pwMode': 100
            },
            {
              'id': 'core1',
              'pwMode': 100
            }
          ]
        }
      ]
    }
  }
````


## Statistics
The following statistics are aggregated via the `/mf/summary` or
`mf/statistics` routes:

| Name of statistic | Comment |
| ----------------- | ------- |
| count                        | Number of experiments stored in the database |
| min                          | Minimum value |
| max                          | Maximum value |
| avg                          | Mean value |
| sum                          | Sum of all values |
| sum_of_squares               | Sum of squares |
| variance                     | Variance |
| std_deviation                | Standard deviation |
| std_deviation_bounds         | Standard deviation bounds (upper and lower) |


The response includes experiment information associated with both the minimum
and maximum value; a response will have the following form:

````
$ curl -XGET http://mf.excess-project.eu:3030/v1/dreamcloud/mf/statistics/rm_stream/task1?metric=execution_time
{
   "workflow": {
      "href": "http://winterfell:3030/v1/v1/dreamcloud/mf/workflows/rm_stream"
   },
   "metric": "execution_time",
   "statistics": {
      "count": 17,
      "min": 20,
      "max": 88,
      "avg": 48.94117647058823,
      "sum": 832,
      "sum_of_squares": 47568,
      "variance": 402.878892733564,
      "std_deviation": 20.07184328191021,
      "std_deviation_bounds": {
         "upper": 89.08486303440864,
         "lower": 8.797489906767815
      }
   },
   "min": {
      "execution_time": 20,
      "platform": "embedded",
      "@timestamp": "2016-07-01T14:17:16.256",
      "host": "winterfell",
      "energy": 72
   },
   "max": {
      "execution_time": 88,
      "platform": "embedded",
      "@timestamp": "2016-07-01T13:46:12.468",
      "host": "winterfell",
      "energy": 132
   }
}
````

