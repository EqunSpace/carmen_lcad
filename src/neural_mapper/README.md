###Currente use of Neural Mapper
####Dependencies
O script de treino da rede com pytorch está no git em src/neural_mapper/pytorch_neural_mapper

Há também um script de testes que abre um modelo treinado e escolhe um indice de entrada pra retornar uma saída (test_model.py).

Utilização da rede:
- Instalar python 3.5 e o pytorch para ele (python padrão do ubuntu 16.04)
- Colocar a pasta 60mts/, em anexo,  em /dados/neural_mapper/
- criar a pasta saved_models/ em (...) /pytorch_neural_mapper/
- Para treinar:
    python3 train.py --batch-size 5 --log-interval 10 --lr 0.001 --epochs 1000
- Para testar:
    python3 test_model.py --model-name <nome_do_modelo> //(Ex. 1000.model)

### Generating input dataset
####Using pytorch code
This code generates the input dataset for neural mapper training.
Como usar:
1 - Criar meta-data a partir de um log com mapa e poses do graphslam usando:
Edite o process-volta_da_ufes_playback_viewer_3D_neural_mapper.ini

Coloque o mapa offline gerado pelo graphslam para ser rodado no map_server:

 ./map_server -map_path ../data/<SEU_MAPA> -map_x 7757721.8 -map_y -363569.5 -block_map on

Coloque seu log que deve ser usado (o mesmo que gerou o mapa)

 ./playback /dados/<LOG_DO_MAPA>

Coloque as poses do graphslam do mapa para serem publicadas:
 ./graphslam_publish poses-<log_>.txt

Verifique se o Publish_poses esta rodando e Dê play no log em uma velocidade de 0.3 ou menos dependendo do seu processador.

- Sobre o código que salva os mapas: Dentro do process o mapper é chamado com as flags para gerar o dataset:

 ./mapper -map_path ../data/maper_test2/ -map_x 7756450 -map_y -364200 -generate_neural_mapper_dataset on -neural_mapper_max_distance_meters 60 -neural_mapper_data_pace 2

-> Parametros:
-generate_neural_mapper_dataset => flag de geracao do dataset (on/off)

-neural_mapper_max_distance_meters => raio de leitura maxima dos lasers (em metros)

-neural_mapper_data_pace => numero 'n' inteiro (a cada n metros e retira uma amostra para o banco de dados)

Obs. 1: Os outros paramatros sao normais do mapper

2 - usar o cogido parse_neural_mapper_metadata.py para gerar o dado processado:
 -> primeiro, alterar a variavel path para a pasta onde se encontra a banco o metadata gerado em 1;
 -> colocar na variavel outpath o caminho onde deseja que o banco de dados processado seja gerado
 -> alterar variavel radius para o raio em metros escolhido quando se gera o metadado
 -> rodar com: python parse_neural_mapper_metadata.py
 
The input dataset are six statistics from velodyne point cloud. Each statistic is a csv file:
	number of points
	max high
	min high
	mean high
	std high
	bean intensity
Each cell in the csv file, correspond to the value inside the cell with resolution R and Width(W) x High(H) defined in meters by command line parameter
The total resolution is WxHxR

#### Deprecated dataset generation (deactivated)
Running
	create the folder where the dataset will be saved
		mkdir /dados/neural_mapper_dataset/
	compile the code:
		cd $CARMEN_HOME/src/neural_mapper/generate_gt/
		make
	Run any process that publish velodyne data
	Run the neural_mapper_dataset_generator
	./neural_mapper_dataset_generator W H R Skip_clouds
	 
